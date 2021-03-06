//----------------------------------------------------
//Copyright (C), 2004-2009,  lst.
//版权所有 (C), 2004-2009,   lst.
//所属模块:flash文件系统
//作者：lst
//版本：V1.0.0
//文件描述:djyfs的windows文件系统驱动接口，
//其他说明:
//修订历史:
//2. ...
//1. 日期: 2009-01-04
//   作者: lst
//   新版本号: V1.0.0
//   修改说明: 原始版本
//------------------------------------------------------

#include "os_inc.h"     

#if (cfg_djyfs == 1) && (cfg_run_mode != cn_mode_tiny))

#include "file.h"
#include "windows.h"

//----移动文件读指针-----------------------------------------------------------
//功能: 把一个文件的读指针移动到一个新位置
//参数: DBX_flash_tag，被操作的flash文件柜的存储媒体标签
//      fp，目标文件指针
//      new_ptr，新的读指针
//返回: true=成功，false=有错误
//-----------------------------------------------------------------------------
bool_t __DFFSD_move_file_read_ptr(struct st_DBX_flash_tag *DBX_flash_tag,
                            djyfs_file *fp,sint64_t new_ptr)
{
    if(new_ptr > fp->file_size)
        return false;

    switch (fp->open_mode)
    {
        case enum_r_rb:
        {
            fp->read_ptr = new_ptr;             //读位置
        }break;
        case enum_w_wb:
        {
            return false;                       //读位置无效
        }break;
        case enum_a_ab:
        {
            return false;                       //读位置无效
        }break;
        case enum_r_rb_plus:
        {
            fp->read_ptr = new_ptr;             //读位置
            fp->write_ptr = new_ptr;            //写位置
        }break;
        case enum_w_wb_plus:
        {
            fp->read_ptr = new_ptr;             //读位置
            fp->write_ptr = new_ptr;            //写位置
        }break;
        case enum_a_ab_plus:
        {
            fp->read_ptr = new_ptr;             //读位置
        }break;
        default: return false;
    }
    return true;
}

//----移动文件写指针-----------------------------------------------------------
//功能: 把一个文件的写指针移动到一个新位置
//参数: DBX_flash_tag，被操作的flash文件柜的存储媒体标签
//      fp，目标文件指针
//      new_ptr，新的写指针
//返回: true=成功，false=有错误
//-----------------------------------------------------------------------------
bool_t __DFFSD_move_file_write_ptr(struct st_DBX_flash_tag *DBX_flash_tag,
                             djyfs_file *fp,sint64_t new_ptr)
{
    switch (fp->open_mode)
    {
        case enum_r_rb:
        {
            return false;                       //只读文件，写位置无效
        }break;
        case enum_w_wb:
        {
            fp->write_ptr = new_ptr;            //写位置
        }break;
        case enum_a_ab:
        {
            fp->write_ptr = new_ptr;            //写位置
        }break;
        case enum_r_rb_plus:
        {
            fp->read_ptr = new_ptr;             //读位置
            fp->write_ptr = new_ptr;            //写位置
        }break;
        case enum_w_wb_plus:
        {
            fp->read_ptr = new_ptr;             //读位置
            fp->write_ptr = new_ptr;            //写位置
        }break;
        case enum_a_ab_plus:
        {
            fp->write_ptr = new_ptr;            //写位置
        }break;
        default: return false;
    }
    if(new_ptr > fp->file_size)       //写指针超出文件长度
    {
        //在FDT表中修改文件长度
       __DFFSD_update_FDT_file_size(DBX_flash_tag,fp,fp->write_ptr);
        fp->file_size = fp->write_ptr;  //用写指针覆盖文件长度
    }
    return true;
}


//----写入flash文件------------------------------------------------------------
//功能：把缓冲区中的数据写入文件，执行下列操作：
//      1、如果写缓冲区有足够的空间，把数据写入写缓冲区即可。
//      2、如果写缓冲区没有足够的空间，连同写缓冲区原有数据和输入缓冲区中的数据
//         一并写入flash
//      3、写入完成后检查读缓冲区是否需要同步并执行之。
//      4、检查FDT表和DDR表是否需要修改并执行之
//参数：buf，输入缓冲区
//      len，本次写入的数据量
//      fp，被操作的文件指针
//返回：本次写入的数据量，不包含写缓冲区中原有的数据
//注:   本函数并没有先调用__DFFSD_write_buf_to_flash函数，再写buf中的内容，那样
//      可能需要多一次调用flash写操作，效率低；而是把写缓冲区的数据与buf中的数据
//      合并一次写入flash.
//-----------------------------------------------------------------------------
uint32_t DFFSD_write_file(struct file_rsc *fp,uint8_t *buf,uint32_t len)
{
    struct flash_chip *chip;
    struct st_DBX_device_tag *DBX_device_tag;
    struct st_DBX_flash_tag *DBX_flash_tag;
    struct file_access_tag access_tag;
    uint32_t completed = 0,write_len,write_sum;
    sint64_t wr_point;
    uint32_t offset_start;
    uint32_t block_no,start_block,end_block,loop,temp;
    uint32_t true_block,last_block;
    uint32_t buffed_data,spare_len; //文件写缓冲区中的数据量和剩余数据量
   
    if((fp==NULL)||(len==0)||(buf==NULL))
        return 0;
    DBX_device_tag = (struct st_DBX_device_tag *)(fp->home_DBX->private_tag);
    //文件柜指针
    DBX_flash_tag = (struct st_DBX_flash_tag *)DBX_device_tag->DBX_medium_tag;
    chip = DBX_flash_tag->chip;     //取文件柜(文件)所属芯片
    buffed_data = ring_check(fp->p_write_buf); //检查缓冲区中的数据量
    if(ring_capacity(fp->p_write_buf) - buffed_data >= len)
    {//缓冲区内有足够的容量，直接把数据写到缓冲区即可。
            completed = ring_write(fp->p_write_buf,(uint8_t*)buf,len);

            completed+=buffed_data;       
    }
    else
    {
        //如果缓冲区没有足够的容量，则把本次需要写入的数据连同缓冲区原来的数据
        //一并写入flash，下面开始执行写入操作。
        //计算连缓冲区中的数据在一起，一共要写入flash的数据量
        write_sum = len + buffed_data;
        spare_len = buffed_data;        //缓冲区剩余数据量
        wr_point = fp->write_ptr - buffed_data;
        //计算起始写位置处于文件内第几块和偏移量
        start_block = (u32)(wr_point /chip->block_size);
        offset_start = (u32)(wr_point % chip->block_size);
        //计算终止写位置处于文件内第几块
        end_block = (u32)((fp->write_ptr + len -1)/chip->block_size);
        __DFFSD_get_access_info(DBX_flash_tag,fp,&access_tag);
        //定位写入点的块号，文件柜内的相对块号。
        block_no = access_tag.write_block_no;
        last_block = access_tag.last_block_no;
        for(loop = start_block; loop <= end_block; loop++)
        {
            if(((wr_point >= access_tag.true_size) && (wr_point != 0)))
            {
                //分配一个新块
                block_no = __DFFSD_allocate_block(DBX_flash_tag);
                if(block_no == cn_limit_uint32) //分配失败(无空闲块)
                    break;
            }
            //计算本次写入的数据长度
            if(write_sum - completed > chip->block_size - offset_start)
                write_len = chip->block_size - offset_start;
            else
                write_len = write_sum - completed;
            if(spare_len != 0)  //写缓冲区里还有数据
            {
                temp = ring_read(fp->p_write_buf,
                                 chip->block_buf+offset_start,write_len);
                spare_len -= temp;  //写缓冲区减去实际读出的数据量
                //实际读出的数据量比write_len少，说明本次读已经把写缓冲区读空。
                if(temp != write_len)
                {
                    //继续从输入缓冲区中拷贝数据
                    memcpy(chip->block_buf+temp+offset_start,
                                buf,write_len-temp);
                }
            }else   //写缓冲区已经空，从输入缓冲区拷贝数据
                memcpy(chip->block_buf+offset_start,
                                       buf+completed-buffed_data,write_len);
            if(wr_point >= access_tag.true_size)    //追加数据
            {
                if(offset_start == 0)   //待写入的块内尚无有效数据
                {
                    //写入目标块
                    true_block = __DFFSD_write_new_block(DBX_flash_tag,block_no,
                                                            0,write_len);
                    if(true_block != cn_limit_uint32)
                    {//正确写入
                        if(wr_point != 0)
                            __DFFSD_add_MAT_item(DBX_flash_tag,last_block,true_block);
                    }else       //写入错误
                        break;
                }else                   //虽是追加，但待写入的块已经有部分有效数据
                {
                    //写入目标块
                    true_block = __DFFSD_write_append_block(DBX_flash_tag,block_no,
                                                    offset_start,write_len);
                    if(true_block != cn_limit_uint32)
                    {//正确写入
                        if(true_block != block_no)   //写入时发生了块替换
                            __DFFSD_update_MAT_item(DBX_flash_tag,block_no,true_block);
                    }else       //写入错误
                        break;
                }
                last_block = true_block;
                access_tag.true_size += write_len;
                wr_point += write_len;
                chip->erase_block(DBX_flash_tag->PCRB_no
                                  + DBX_flash_tag->start_block);
            }else   //在文件的中间写入，若需擦除就需要使用PCRB块进行保护
            {
                //写入目标块
                true_block = __DFFSD_write_update_block(DBX_flash_tag,block_no,
                                                offset_start,write_len);
                if(true_block != cn_limit_uint32)
                {//正确写入
                    if(true_block != block_no)   //写入时发生了块替换
                        __DFFSD_update_MAT_item(DBX_flash_tag,block_no,true_block);
                }else       //写入错误
                    break;
                //确定下一循环写入点的块号，文件柜内的相对块号。
                block_no = DBX_flash_tag->DDR_MAT[block_no].next;
                wr_point += write_len;
                if(wr_point > access_tag.true_size)
                    access_tag.true_size = wr_point;
                chip->erase_block(DBX_flash_tag->PCRB_no
                                  + DBX_flash_tag->start_block);
            }
            completed += write_len;
            offset_start = 0;   //第二块开始，起始偏移地址肯定是0
        }
    }
    if((completed - buffed_data) > 0)      //写入有数据
    {
        __DFFSD_sync_buf_after_write(DBX_flash_tag,fp,completed-buffed_data);
        __DFFSD_move_file_write_ptr(DBX_flash_tag,fp,
                                    fp->write_ptr + completed-buffed_data);
    }
    __DFFSD_write_DDR(DBX_flash_tag);//如果DDR表有变化，写入flash中
    return completed - buffed_data;
}
//功能；把文件中的数据读入到缓冲区中，执行以下操作
//      1、看读缓冲区中有没有数据，有则读入到接收缓冲区中
//      2、若读缓冲区中的数据量不足或没有数据，则从flash中补充读取，并按照不越块
//         边界为限最大限度地填写读缓冲区
//      3、读出完成后看是否需要同步写缓冲区并执行之
//参数：buf，输入缓冲区
//      len，读出的数据量
//      fp，被操作的文件指针
//返回：实际读取的字节数
//-----------------------------------------------------------------------------
uint32_t DFFSD_read_file(struct file_rsc *fp,uint8_t *buf,uint32_t len)
{
    struct flash_chip *chip;
    struct st_DBX_device_tag *DBX_device_tag;
    struct st_DBX_flash_tag *DBX_flash_tag;
    uint32_t completed = 0,read_len,result_len;
    uint32_t offset_start;
    uint32_t block_no,start_block,end_block,loop;
    uint32_t read_result;
    if((fp==NULL)||(len==0)||(buf==NULL))
        return 0;
    DBX_device_tag = (struct st_DBX_device_tag *)(fp->home_DBX->private_tag);
    //文件柜指针
    DBX_flash_tag = (struct st_DBX_flash_tag *)DBX_device_tag->DBX_medium_tag;
    __DFFSD_sync_buf_before_read(DBX_flash_tag,fp,len);
    chip = DBX_flash_tag->chip;     //取文件柜(文件)所属芯片
    //计算起始读写位置处于文件内第几块
    start_block = (u32)(fp->read_ptr/chip->block_size);
    //计算终止读写位置处于文件内第几块
    end_block = (u32)((fp->read_ptr + len -1)/chip->block_size);
    offset_start = fp->read_ptr % chip->block_size;
    if(fp->read_ptr + len >fp->file_size)
        result_len = (u32)(fp->file_size - fp->read_ptr);
    else
        result_len = len;
    //定位读出点的块号，文件柜内的相对块号。
    block_no = __DFFSD_locate_block(DBX_flash_tag,fp,fp->read_ptr);
    for(loop = start_block; loop <= end_block; loop++)
    {
        if(result_len - completed > chip->block_size - offset_start)
            read_len = chip->block_size - offset_start;
        else
            read_len = result_len - completed;
        //读出目标块
        read_result =chip->read_data_with_ecc(
                                block_no + DBX_flash_tag->start_block,
                                offset_start,buf+completed,read_len);
        if(read_result == 2)    //读出错误，坏块
        {
            return completed;
        }
        completed += read_len;
        offset_start = 0;   //第二块开始，起始偏移地址肯定是0
        block_no = DBX_flash_tag->DDR_MAT[block_no].next;
    }
    fp->read_ptr += completed;
    return completed;
}

//----刷写缓冲区---------------------------------------------------------------
//功能: 把文件写缓冲区的数据写入flash
//参数：fp，被操作的文件指针
//返回：实际写入flash的字节数
//-----------------------------------------------------------------------------
uint32_t DFFSD_flush_file(struct file_rsc *fp)
{
    struct st_DBX_device_tag *DBX_device_tag;
    struct st_DBX_flash_tag *DBX_flash_tag;
    if(fp==NULL)
        return 0;
    DBX_device_tag = (struct st_DBX_device_tag *)(fp->home_DBX->private_tag);
    //文件柜指针
    DBX_flash_tag = (struct st_DBX_flash_tag *)DBX_device_tag->DBX_medium_tag;
    return __DFFSD_write_buf_to_flash(DBX_flash_tag,fp);
}

//----查询文件可读数据量-------------------------------------------------------
//功能：查询一个文件内有多少可读数据，对于固态文件（如flash文件）来说，就是文件
//      长度-当前指针，但流的标准定义并非一定如此，比如通信端子。而且，文件读写
//      指针如何表示与具体文件系统驱动的实现有关，文件系统的核心程序不应该做任何
//      假设。
//参数：fp，被操作的文件指针
//返回：文件可读数据的字节数
//-----------------------------------------------------------------------------
sint64_t DFFSD_query_file_stocks(struct file_rsc *fp)
{
    struct st_DBX_device_tag *DBX_device_tag;
    struct st_DBX_flash_tag *DBX_flash_tag;
    struct fdt_info FDT_item;
    
    if(fp == NULL)
        return 0;
    DBX_device_tag = (struct st_DBX_device_tag *)(fp->home_DBX->private_tag);
    DBX_flash_tag = (struct st_DBX_flash_tag*)DBX_device_tag->DBX_medium_tag;
    __DFFSD_read_FDT_item(DBX_flash_tag,
                          (uint32_t)fp->file_medium_tag,&FDT_item);
    if(FDT_item.mattr & cn_FS_ATTR_DIRECTORY)
        return 0;       //目录不能读
    return fp->file_size - fp->read_ptr;
}


//----查询文件可写数据量-------------------------------------------------------
//功能：查询一个文件还能写入多少数据，对于固态文件（如flash文件），可能就是文件
//      柜的剩余空间，但流的标准定义并非一定如此，比如通信端子。而且，文件柜剩余
//      空间如何表示与具体文件系统驱动的实现有关，文件系统的核心程序不应该做任何
//      假设。不能认为DBX_flash_tag->free_sum就是文件仍可写入的数据量。
//      flash文件系统的计算方式是:
//      1、文件已分配的块的容量-文件尺寸。
//      2、加上文件柜剩余空间DBX_flash_tag->free_sum。
//      3、减去文件缓冲区中已有的数据量
//参数：fp，被操作的文件指针
//返回：文件还能写入的字节数
//-----------------------------------------------------------------------------
sint64_t DFFSD_query_file_cubage(struct file_rsc *fp)
{
    struct st_DBX_device_tag *DBX_device_tag;
    struct st_DBX_flash_tag *DBX_flash_tag;
    struct flash_chip *chip;
    struct fdt_info FDT_item;
    if(fp == NULL)
        return 0;
    DBX_device_tag = (struct st_DBX_device_tag *)(fp->home_DBX->private_tag);
    DBX_flash_tag = (struct st_DBX_flash_tag*)DBX_device_tag->DBX_medium_tag;
    chip = DBX_flash_tag->chip;
    __DFFSD_read_FDT_item(DBX_flash_tag,
                          (uint32_t)fp->file_medium_tag,&FDT_item);
    if((FDT_item.mattr & cn_FS_ATTR_READ_ONLY)
                || (FDT_item.mattr & cn_FS_ATTR_DIRECTORY))
        return 0;       //只读文件或目录不能写入

    return DBX_flash_tag->free_sum * chip->block_size;  // - buffed_size + size;
}

//----设置文件长度-------------------------------------------------------------
//功能：设置文件长度，短于实际长度将截尾，长于实际长度将追加空间。本函数的主要
//      作用在于在正式写入数据之前，预先分配存储器。
//参数：fp，被操作的文件指针
//      new_size，新的文件长度。
//返回：新的文件长度，若与new_size参数不等，一般是因为文件柜没有足够的容量导致
//-----------------------------------------------------------------------------
sint64_t __DFFSD_set_file_size(struct file_rsc *fp,sint64_t new_size)
{
    struct st_DBX_device_tag *DBX_device_tag;
    sint64_t result,temp;
    uint32_t origin_blocks,new_blocks,last_block_no,next_block_no;
    uint32_t loop;
    struct st_DBX_flash_tag *DBX_flash_tag;
    struct flash_chip *chip;
    struct fdt_info FDT_item;
    //读父目录的FDT_item
    if(fp == NULL)
        return 0;
    if(new_size == fp->file_size)
        return new_size;    //新的文件长度与原长度相等，直接返回。
    DBX_device_tag = (struct st_DBX_device_tag *)(fp->home_DBX->private_tag);
    DBX_flash_tag = (struct st_DBX_flash_tag*)DBX_device_tag->DBX_medium_tag;
    chip = DBX_flash_tag->chip;
    __DFFSD_read_FDT_item(DBX_flash_tag,
                          (uint32_t)fp->file_medium_tag,&FDT_item);
    if((FDT_item.mattr & cn_FS_ATTR_READ_ONLY)
                || (FDT_item.mattr & cn_FS_ATTR_DIRECTORY))
        return 0;       //只读文件或目录不能执行设置长度的操作
    //下面两个if用于确定原文件占用的块数和新文件长度所需的块数
    if(fp->file_size == 0)
        origin_blocks = 1;
    else
        origin_blocks = (u32)((fp->file_size + chip->block_size -1)/chip->block_size);
    if(new_size < 0)
        new_size = 0;
    if(new_size == 0)
        new_blocks = 1;
    else
        new_blocks = (u32)((new_size + chip->block_size -1)/chip->block_size);
    last_block_no = __DFFSD_last_block(DBX_flash_tag,fp);
    if(origin_blocks < new_blocks)     //文件占用块数需增加
    {
        for(loop = origin_blocks; loop < new_blocks; loop++)
        {
            next_block_no = __DFFSD_allocate_block(DBX_flash_tag);
            if(next_block_no != cn_limit_uint32)
            {
                __DFFSD_add_MAT_item(DBX_flash_tag,last_block_no,next_block_no);
                last_block_no = next_block_no;
            }else
                break;
        }
        if(loop == new_blocks)
            result = new_size;
        else
            result = (sint64_t)loop * chip->block_size;
    }else if(origin_blocks > new_blocks)     //文件占用块数需减少
    {
        result = new_size;
        for(loop = origin_blocks; loop > new_blocks; loop--)
        {
            next_block_no = last_block_no;
            last_block_no = DBX_flash_tag->DDR_MAT[next_block_no].previous;
            __DFFSD_free_block(DBX_flash_tag,next_block_no);
        }
        //最后一块的向后指针指向自己
        DBX_flash_tag->DDR_MAT[last_block_no].next = last_block_no;
    }else           //占用块数不变
        result = new_size;
    fp->file_size = result;
    //以下处理读写缓冲区，但并不处理读写指针，仅把非法指针(因文件缩短而超出文件
    //长度的指针位置)移到合法位置。
    if(fp->p_read_buf != NULL)      //读缓冲区指针非空，说明有读缓冲区
    {
        if(fp->read_ptr > result)   //读指针在文件尺寸后面
        {
            ring_flush(fp->p_read_buf); //清空读缓冲区
            fp->read_ptr = result;      //读指针回到文件末
        }else
        {
            temp = fp->read_ptr + ring_check(fp->p_read_buf);
            //读指针未超出文件长度但读缓冲区中数据超出文件长度，将超出部分砍掉
            if(temp > result)
            {
                ring_skip_tail(fp->p_read_buf,(u32)(temp-result));
            }
        }
    }
    if(fp->p_write_buf != NULL)     //写缓冲区指针非空，说明有写缓冲区
    {
        if(fp->write_ptr > result)  //写指针超出文件长度
        {
            temp = result - fp->write_ptr;  //计算超出数量
            ring_skip_tail(fp->p_write_buf,(u32)temp);   //砍掉超出文件长度的并且可能
                                                    //还在缓冲区中的数据。
            fp->write_ptr = result; //写指针回到文件末
        }else
        {
            //写指针没有超出文件长度，无需处理
        }
    }
   __DFFSD_update_FDT_file_size(DBX_flash_tag,fp,result);
   __DFFSD_write_DDR(DBX_flash_tag);   //可能修改DDR,写入flash
    return result;
}

//----设置文件指针-------------------------------------------------------------
//功能：把文件指针设置到指定的位置，并相应地同步读写缓冲区。
//参数：fp，被操作的文件指针
//      offset，移动的位置
//      whence，从何处开始计算移动长度，参见SEEK_CUR等的定义
//返回：0=成功，-1=失败
//-----------------------------------------------------------------------------
uint32_t DFFSD_seek_file(struct file_rsc *fp,struct seek_para *position)
{
    struct st_DBX_device_tag *DBX_device_tag;
    struct st_DBX_flash_tag *DBX_flash_tag;
    sint64_t new_position,temp;
    uint32_t buffed_size;
    uint8_t  whence;
    if((position == NULL) ||(fp == NULL))
        return -1;
    DBX_device_tag = (struct st_DBX_device_tag *)(fp->home_DBX->private_tag);
    whence = position->whence;
    DBX_flash_tag = (struct st_DBX_flash_tag*)DBX_device_tag->DBX_medium_tag;
    switch (fp->open_mode)
    {
        case enum_r_rb:           //只读文件，需移动读指针和读缓冲区
        case enum_a_ab_plus:      //写追加方式，写指针不能变，读指针的方式与
                                //只读文件一致
        {
            if(whence == SEEK_SET)
                new_position = position->offset;
            else if(whence == SEEK_CUR)
                new_position = position->offset + fp->read_ptr;
            else if(whence == SEEK_END)
                new_position = fp->file_size + position->offset;
            else
                return -1;
            temp = new_position - fp->read_ptr;
            buffed_size = ring_check(fp->p_read_buf);
            if((temp > 0) && (temp < buffed_size))
                //往后移且移动量比缓冲区中的数据少
                ring_dumb_read(fp->p_read_buf,(ptu32_t)temp);
            else
                ring_flush(fp->p_read_buf);//往前移或者移动量大于缓冲区中数据量
            if(new_position > fp->file_size)
                fp->read_ptr = fp->file_size;
            else if(new_position < 0)
                fp->read_ptr = 0;
            else
                fp->read_ptr = new_position;
        }break;
        case enum_w_wb:   //只写文件，需移动写指针
        {
            __DFFSD_write_buf_to_flash(DBX_flash_tag,fp);
            if(whence == SEEK_SET)
                new_position = position->offset;
            else if(whence == SEEK_CUR)
                new_position = position->offset + fp->write_ptr;
            else if(whence == SEEK_END)
                new_position = fp->file_size - position->offset;
            else
                return -1;
            if(new_position > fp->file_size)
            {
                //延长文件，分配更多存储器，但不填充新增加的存储器
                if(__DFFSD_set_file_size(fp,new_position) != new_position)
                    return -1;
                else
                {
                    fp->write_ptr = new_position;   //设置写指针
                }
            }else if(new_position < 0)
                fp->write_ptr = 0;
            else
                fp->write_ptr = new_position;
        }break;
        case enum_a_ab:   //追加写方式，seek操作无效
        {
        }break;
        case enum_r_rb_plus:  //这两者都是可读写方式，读写指针重合，处理方式相同
        case enum_w_wb_plus:
        {
            if(whence == SEEK_SET)
                new_position = position->offset;
            else if(whence == SEEK_CUR)
                new_position = position->offset + fp->read_ptr;
            else if(whence == SEEK_END)
                new_position = fp->file_size + position->offset;
            else
                return -1;
            __DFFSD_write_buf_to_flash(DBX_flash_tag,fp);//写缓冲区数据必须写入

            if(new_position > fp->file_size)    //新位置超出文件长度
            {
                //延长文件，分配更多存储器，但不初始化新增加的存储器
                if(__DFFSD_set_file_size(fp,new_position) != new_position)
                    return -1;
                else
                {
                    fp->write_ptr = new_position;   //设置写指针
                    fp->read_ptr = new_position;    //设置读指针
                }
            }else if(new_position < 0)  //新位置小于0，设为0
            {
                fp->write_ptr = 0;      //写缓冲区已经写入，处理写指针即可
                fp->read_ptr = 0;    //设置读指针，读指针和读缓冲区可以独立设置
                ring_flush(fp->p_read_buf); //清空读缓冲区
            }
            else
            {
                fp->write_ptr = new_position;//写缓冲区已经写入，处理写指针即可
                fp->read_ptr = new_position;    //设置读指针
                if(new_position < fp->read_ptr) //新位置使读指针后移(朝文件首)
                {
                    ring_flush(fp->p_read_buf); //缓冲区中数据全部作废
                }else
                {
                    //计算缓冲区结束处处于文件的什么位置
                    temp = fp->read_ptr + ring_check(fp->p_read_buf);
                    if(new_position < temp)
                    {//读指针前移，但未超出读缓冲区覆盖的范围。
                        //缓冲区中数据部分作废
                        ring_dumb_read(fp->p_read_buf,(u32)(temp-new_position));
                    }else
                    {//读指针前移，但超出读缓冲区覆盖的范围。
                        ring_flush(fp->p_read_buf);     //缓冲区中数据全部作废
                    }
                }
            }
        }break;
        default: return -1;
    }
    return 0;
}

//----建立项目（文件/目录）----------------------------------------------------
//功能：建立文件(目录)，attr参数将表明建立目录还是文件。如果建立文件，则同时分配
//      1块存储器。删除文件内容使文件长度为0时，也不会把最后一块释放。总之，任何
//      情况下，文件至少占用1块存储器
//参数：name，目录（文件）名
//      parent，父目录指针。
//      attr，新文件（目录）的属性
//返回：true=成功，false=失败，一般是因为文件柜没有足够的容量导致
//特注：本函数不检查重名，由调用者保证。作为存储介质，专守存储数据的职责，是否
//      允许重名是上一层的责任。
//-----------------------------------------------------------------------------
bool_t DFFSD_create_file(char *name,struct file_rsc *parent,
                         union file_attrs attr)
{
    struct st_DBX_device_tag *DBX_device_tag;
    struct flash_chip *chip;
    uint32_t FDT_item_no_parent,FDT_item_no_eldest,FDT_item_no_least;
    uint32_t FDT_item_no_me,block_me,temp;
    struct fdt_info FDT_item_me,FDT_item_parent,FDT_item_eldest,FDT_item_least;
    struct st_DBX_flash_tag *DBX_flash_tag;

    if(parent == NULL)
        return false;
    DBX_device_tag =(struct st_DBX_device_tag *)(parent->home_DBX->private_tag);
    DBX_flash_tag = (struct st_DBX_flash_tag*)DBX_device_tag->DBX_medium_tag;
    chip = DBX_flash_tag->chip;
    FDT_item_no_parent = (uint32_t)parent->file_medium_tag;
    //读父目录的FDT条目
    __DFFSD_read_FDT_item(DBX_flash_tag,FDT_item_no_parent,&FDT_item_parent);
    if(!(FDT_item_parent.mattr & cn_FS_ATTR_DIRECTORY))
        return false;       //parent不是一个目录，而是文件
    FDT_item_no_me = __DFFSD_allocate_FDT_item(DBX_flash_tag);//分配一个FDT条目
    //debug 要不要对分配的FDT进行检测，有可能已经被使用了。如果被使用了，说明
    //FDT表乱了，不能挽救了。所以不能让分配的出现问题。这就必须在创建的时候，小心
    //再小心，千万不能建立成功了，而没有从空闲链表弄好。
    if(FDT_item_no_me == cn_limit_uint32)   //分配不成功
        return false;
    if(FDT_item_no_parent == 0)
    {
        //父目录是根目录，分配时FDT条目时会修改根目录条目，故需重读
        __DFFSD_read_FDT_item(DBX_flash_tag,0,&FDT_item_parent);
    }
    if( ! attr.bits.folder)
    {
        block_me = __DFFSD_allocate_block(DBX_flash_tag);
        if(block_me == cn_limit_uint32)
        {
            __DFFSD_free_FDT_item(DBX_flash_tag,FDT_item_no_me);
            return false;
        }else
        {
            __fill_little_32bit(FDT_item_me.fstart_dson,0,block_me);
            DBX_flash_tag->DDR_MAT[block_me].attr = cn_file_block_start;
            DBX_flash_tag->DDR_MAT[block_me].previous = FDT_item_no_me;
            DBX_flash_tag->DDR_MAT[block_me].next = block_me;
            //计算last_block_no在MAT表中位置处在DDR表中是第几块
            temp = __DFFSD_MAT_start(DBX_flash_tag);
            temp=(temp + block_me*4*2) / chip->block_size;
            //DDR_bak中标记该块为未写入
            DBX_flash_tag->writed_DDR_bak[temp] = false;
            //DDR_main中标记该块为未写入
            DBX_flash_tag->writed_DDR_main[temp] = false;
        }
    }else
    {
        block_me = cn_limit_uint32;
        __fill_little_32bit(FDT_item_me.fstart_dson,0,cn_limit_uint32);
    }
    __DFFSD_asm_FDT_item(&FDT_item_me,attr,name);
    __fill_little_32bit(FDT_item_me.parent,0,FDT_item_no_parent);
    FDT_item_no_eldest = __pick_little_32bit(FDT_item_parent.fstart_dson,0);
    if(FDT_item_no_eldest == cn_limit_uint32)   //父目录是空目录
    {
        //构建新建目录项
        __fill_little_32bit(FDT_item_me.previous,0,FDT_item_no_me);
        __fill_little_32bit(FDT_item_me.next,0,FDT_item_no_me);
        //新建目录项写入flash
        __DFFSD_write_FDT_item(DBX_flash_tag,FDT_item_no_me,&FDT_item_me);
        //父目录项的子项指向新建目录项
        __fill_little_32bit(FDT_item_parent.fstart_dson,0,FDT_item_no_me);
        __DFFSD_write_FDT_item(DBX_flash_tag,FDT_item_no_parent,
                               &FDT_item_parent);   //父目录修改后写回flash
    }else       //父目录项有子项(包括目录和文件)
    {
        __DFFSD_read_FDT_item(DBX_flash_tag,FDT_item_no_eldest,&FDT_item_eldest);
        FDT_item_no_least = __pick_little_32bit(FDT_item_eldest.previous,0);
        //构建新建目录项
        __fill_little_32bit(FDT_item_me.previous,0,FDT_item_no_least);
        __fill_little_32bit(FDT_item_me.next,0,FDT_item_no_eldest);
        //新建目录项写入flash
        __DFFSD_write_FDT_item(DBX_flash_tag,FDT_item_no_me,&FDT_item_me);
        if(FDT_item_no_least == FDT_item_no_eldest) //父目录项只有一个子项
        {
            //该子项的前后指针均指向新建目录项
            __fill_little_32bit(FDT_item_eldest.previous,0,FDT_item_no_me);
            __fill_little_32bit(FDT_item_eldest.next,0,FDT_item_no_me);
            //该子项写回flash
            __DFFSD_write_FDT_item(DBX_flash_tag,
                                   FDT_item_no_eldest,&FDT_item_eldest);
        }else       //父目录项有多个子项
        {
            //第一个子项的前指针指向新建目录项
            __fill_little_32bit(FDT_item_eldest.previous,0,FDT_item_no_me);
            //第一个子项写回flash
            __DFFSD_write_FDT_item(DBX_flash_tag,
                                   FDT_item_no_eldest,&FDT_item_eldest);
            //读出最后一个子项
            __DFFSD_read_FDT_item(DBX_flash_tag,
                                  FDT_item_no_least,&FDT_item_least);
            //新建子项的后指针指向新建目录项
            __fill_little_32bit(FDT_item_least.next,0,FDT_item_no_me);
            //最后一个子项写回flash
            __DFFSD_write_FDT_item(DBX_flash_tag,
                                   FDT_item_no_least,&FDT_item_least);
        }
    }
    __DFFSD_write_DDR(DBX_flash_tag);
    return true;
}

//----建立项目（文件/目录）----------------------------------------------------
//功能：建立文件(目录)，attr参数将表明建立目录还是文件。如果建立文件，则同时分配
//      1块存储器。删除文件内容使文件长度为0时，也不会把最后一块释放。总之，任何
//      情况下，文件至少占用1块存储器
//参数：name，目录（文件）名
//      parent，父目录指针。
//      attr，新文件（目录）的属性
//返回：true=成功，false=失败，一般是因为文件柜没有足够的容量导致
//特注：本函数不检查重名，由调用者保证。作为存储介质，专守存储数据的职责，是否
//      允许重名是上一层的责任。
//-----------------------------------------------------------------------------
bool_t DFFSD_move_file(struct file_rsc *src_fp,	struct file_rsc *dest_fp)
{
    struct st_DBX_device_tag *DBX_device_tag;
    uint32_t src_FDT_item_no,dest_FDT_item_no,dest_FDT_item_no_parent;
    uint32_t FDT_item_no_previous,FDT_item_no_next;
    struct fdt_info src_FDT_item,dest_FDT_item;
    struct fdt_info FDT_item_previous,FDT_item_next;
    struct st_DBX_flash_tag *DBX_flash_tag;

    if(dest_fp == NULL || src_fp == NULL)
        return false;    
    DBX_device_tag =(struct st_DBX_device_tag *)(src_fp->home_DBX->private_tag);
    DBX_flash_tag = (struct st_DBX_flash_tag*)DBX_device_tag->DBX_medium_tag;    
    
    src_FDT_item_no = (uint32_t)src_fp->file_medium_tag;
    dest_FDT_item_no = (uint32_t)dest_fp->file_medium_tag;
    //读父目录的FDT条目
    __DFFSD_read_FDT_item(DBX_flash_tag,src_FDT_item_no,&src_FDT_item);
    __DFFSD_read_FDT_item(DBX_flash_tag,dest_FDT_item_no,&dest_FDT_item);
    if((src_FDT_item.mattr & cn_FS_ATTR_DIRECTORY)||
                                (dest_FDT_item.mattr & cn_FS_ATTR_DIRECTORY))
        return false;       //是目录

    dest_FDT_item_no_parent = __pick_little_32bit(dest_FDT_item.parent,0); 
    __fill_little_32bit(src_FDT_item.parent,0,dest_FDT_item_no_parent);
	__DFFSD_write_FDT_item(DBX_flash_tag,src_FDT_item_no,&src_FDT_item);
    
    

	FDT_item_no_previous = __pick_little_32bit(dest_FDT_item.previous,0);
 	FDT_item_no_next = __pick_little_32bit(dest_FDT_item.next,0);
    //被移动的只有一项
	if((FDT_item_no_previous == src_FDT_item_no)&&(FDT_item_no_next == src_FDT_item_no))
    {
        __DFFSD_free_FDT_item(DBX_flash_tag,dest_FDT_item_no);
                //构建新建目录项
        __fill_little_32bit(src_FDT_item.previous,0,src_FDT_item_no);
        __fill_little_32bit(src_FDT_item.next,0,src_FDT_item_no);
        //新建目录项写入flash
        __DFFSD_write_FDT_item(DBX_flash_tag,src_FDT_item_no,&src_FDT_item); 
        //只有一项，没什么好处理	
    }else 
    {    
     	if((FDT_item_no_previous != src_FDT_item_no)&&(FDT_item_no_previous==FDT_item_no_next))  
        {//被移动的，有一项兄弟目录（文件）
            //把另外一项的前指针和后指针，都指向自己，FDT_item_no_previous和FDT_item_no_next
            //是同一项，随便读一项
         	__DFFSD_read_FDT_item(DBX_flash_tag,
                                  FDT_item_no_previous,&FDT_item_previous);
            __fill_little_32bit(FDT_item_previous.previous,0,FDT_item_no_previous);
            __fill_little_32bit(FDT_item_previous.next,0,FDT_item_no_previous);
           __DFFSD_write_FDT_item(DBX_flash_tag,FDT_item_no_previous,&FDT_item_previous);
        }
    	else
        {
        	__DFFSD_read_FDT_item(DBX_flash_tag,
                                  FDT_item_no_previous,&FDT_item_previous);
        	__DFFSD_read_FDT_item(DBX_flash_tag,
                                  FDT_item_no_next,&FDT_item_next);
            //被移动项的前项目的后指针，指向被移动项的后项目
        	__fill_little_32bit(FDT_item_previous.next,0,FDT_item_no_next);
            //被移动项的后项目的前指针，指向被移动项的前项目
            __fill_little_32bit(FDT_item_next.previous,0,FDT_item_no_previous);
        	__DFFSD_write_FDT_item(DBX_flash_tag,FDT_item_no_previous,&FDT_item_previous);
        	__DFFSD_write_FDT_item(DBX_flash_tag,FDT_item_no_next,&FDT_item_next);
        }
    }

     
	FDT_item_no_previous = __pick_little_32bit(src_FDT_item.previous,0);
 	FDT_item_no_next = __pick_little_32bit(src_FDT_item.next,0);
    //被移动的只有一项
	if((FDT_item_no_previous == src_FDT_item_no)&&(FDT_item_no_next == src_FDT_item_no))
    {
        //只有一项，没什么好处理	
    }else 
    {    
     	if((FDT_item_no_previous != src_FDT_item_no)&&(FDT_item_no_previous==FDT_item_no_next))  
        {//被移动的，有一项兄弟目录（文件）
            //把另外一项的前指针和后指针，都指向自己，FDT_item_no_previous和FDT_item_no_next
            //是同一项，随便读一项
         	__DFFSD_read_FDT_item(DBX_flash_tag,
                                  FDT_item_no_previous,&FDT_item_previous);
            __fill_little_32bit(FDT_item_previous.previous,0,FDT_item_no_previous);
            __fill_little_32bit(FDT_item_previous.next,0,FDT_item_no_previous);
           __DFFSD_write_FDT_item(DBX_flash_tag,FDT_item_no_previous,&FDT_item_previous);
        }
    	else
        {
        	__DFFSD_read_FDT_item(DBX_flash_tag,
                                  FDT_item_no_previous,&FDT_item_previous);
        	__DFFSD_read_FDT_item(DBX_flash_tag,
                                  FDT_item_no_next,&FDT_item_next);
            //被移动项的前项目的后指针，指向被移动项的后项目
        	__fill_little_32bit(FDT_item_previous.next,0,FDT_item_no_next);
            //被移动项的后项目的前指针，指向被移动项的前项目
            __fill_little_32bit(FDT_item_next.previous,0,FDT_item_no_previous);
        	__DFFSD_write_FDT_item(DBX_flash_tag,FDT_item_no_previous,&FDT_item_previous);
        	__DFFSD_write_FDT_item(DBX_flash_tag,FDT_item_no_next,&FDT_item_next);
        }
    }

    __DFFSD_write_DDR(DBX_flash_tag);//ddr也许改变，若无改变，不会真的写
    return true;
}

//----检查文件夹的子文件（目录）数---------------------------------------------
//功能：检查文件夹的子文件（目录）数，不包含子目录的子条目
//参数：DBX_device_tag，被操作的文件柜指针，通用结构，非flash专用。
//      fp，目标文件夹的文件指针
//返回：子文件（目录）数
//-----------------------------------------------------------------------------
uint32_t DFFSD_check_folder(struct file_rsc *parent)
{
    struct st_DBX_device_tag *DBX_device_tag;
    struct fdt_info FDT_item;
    struct st_DBX_flash_tag *DBX_flash_tag;
    uint32_t FDT_item_no,FDT_item_no_next;
    uint32_t result;
    if(parent == NULL)
        return 0;
    if( ! parent->attr.bits.folder)      //这是文件而不是目录
        return 0;
    result = 0;
    DBX_device_tag = (struct st_DBX_device_tag *)(parent->home_DBX->private_tag);
    DBX_flash_tag = (struct st_DBX_flash_tag *)DBX_device_tag->DBX_medium_tag;
    FDT_item_no = (uint32_t)parent->file_medium_tag;
    __DFFSD_read_FDT_item(DBX_flash_tag,FDT_item_no,&FDT_item);
    //第一个子条目的FDT条目号
    FDT_item_no = __pick_little_32bit(FDT_item.fstart_dson,0);
    if(FDT_item_no == cn_limit_uint32)
        return 0;
    FDT_item_no_next = FDT_item_no;
    do
    {
        result++;
        __DFFSD_read_FDT_item(DBX_flash_tag,FDT_item_no_next,&FDT_item);
        FDT_item_no_next = __pick_little_32bit(FDT_item.next,0);
    }while(FDT_item_no != FDT_item_no_next);
    return result;
}

//----删除项目（文件/目录）---------------------------------------------------
//功能：删除一个文件(目录)，只能删除空目录或已经关闭的文件。
//参数：fp，被删除的目录（文件）指针
//返回：true=成功，false=失败
//-----------------------------------------------------------------------------
bool_t DFFSD_remove_file(struct file_rsc *fp)
{
    struct fdt_info FDT_item;
    struct st_DBX_device_tag *DBX_device_tag;
    struct st_DBX_flash_tag *DBX_flash_tag;
    struct flash_chip *chip;
    uint32_t FDT_item_no,loop,block_no,block_no_next,blocks;
    if(fp == NULL)
        return false;
    if(( ! fp->attr.bits.folder) && (fp->open_counter != 0))
        return false;   //fp是一个已经打开的文件
    if(fp->attr.bits.folder)
    {
        if(DFFSD_check_folder(fp) != 0)
            return false;   //是一个非空目录
    }
    DBX_device_tag = (struct st_DBX_device_tag *)(fp->home_DBX->private_tag);
    DBX_flash_tag = (struct st_DBX_flash_tag *)DBX_device_tag->DBX_medium_tag;
    chip = DBX_flash_tag->chip;
    FDT_item_no = (uint32_t)fp->file_medium_tag;
    if( ! fp->attr.bits.folder) //是个文件，需要释放文件占用的存储器
    {
        if(fp->file_size == 0)  //0长度文件也占用1块
            blocks = 1;
        else
            blocks = (u32)((fp->file_size + (s64)chip->block_size -1) / chip->block_size);
        __DFFSD_read_FDT_item(DBX_flash_tag,FDT_item_no,&FDT_item);//取目录项
        block_no = __pick_little_32bit(FDT_item.fstart_dson,0);//取文件首块块号
        for(loop = 0; loop < blocks; loop++)
        {
            //保存下一块块号
            block_no_next = DBX_flash_tag->DDR_MAT[block_no].next;
            __DFFSD_free_block(DBX_flash_tag,block_no); //释放一块
            block_no = block_no_next;
        }
    }
    __DFFSD_free_FDT_item(DBX_flash_tag,FDT_item_no); //释放FDT项
    
    __DFFSD_write_DDR(DBX_flash_tag); 
    return true;
}

//----遍历一个目录的子项-------------------------------------------------------
//功能: 从current_file开始,获取下parent的下一个文件的指针,直到parent
//      的子项搜索完毕.
//参数: parent,需要搜索的树枝的祖先结点
//      current_file,当前搜索位置,空则表示第一次查找
//返回: 搜索到的子项指针(可能是文件或目录)
//备注: 这个函数，可以作为windows下 FindFirstFile等三个函数的配合遍历整
//      个目录下的文件
//      第一次遍历时(以current==NULL)为标志，系统将为struct file_rsc结构分配内存，
//      最后一次，将释放内存。如果用户正确地调用DFFSD_item_traversal_son遍历完
//      整个目录，内存管理将完全正常，否则，为struct file_rsc结构分配的内存将需
//      要用户手动释放
//---------------------------------------------------------------------
struct file_rsc * DFFSD_item_traversal_son(struct file_rsc *parent,
                                        	struct file_rsc *current)
{
	uint32_t FDT_item_no,first_FDT_item_no;
	struct fdt_info FDT_item;
	struct st_DBX_device_tag *DBX_device_tag;
	struct st_DBX_flash_tag *DBX_flash_tag;

    if(parent == NULL)
        return NULL;
    if(!parent->attr.bits.folder)       //父目录是个文件。
        return NULL;
    if(parent->file_medium_tag == cn_limit_uint32)
        return NULL;
    
	DBX_device_tag =(struct st_DBX_device_tag *)(parent->home_DBX->private_tag);
	DBX_flash_tag = (struct st_DBX_flash_tag*)DBX_device_tag->DBX_medium_tag;
   	__DFFSD_read_FDT_item(DBX_flash_tag,parent->file_medium_tag,&FDT_item);
	first_FDT_item_no = __pick_little_32bit(FDT_item.fstart_dson,0);
    if(first_FDT_item_no == cn_limit_uint32)    //父目录是个空目录
        return NULL;

    if(current == NULL)    //如果curent_file是空的，返回第一个子项
    {
        current = m_malloc(sizeof(struct file_rsc),cn_timeout_forever);
        if(current == NULL)
            return NULL;
    	__DFFSD_read_FDT_item(DBX_flash_tag,first_FDT_item_no,&FDT_item);
    	__DFFSD_uasm_FDT_item(DBX_flash_tag,current,&FDT_item);
        current->home_DBX = parent->home_DBX;
    	current->file_medium_tag = (ptu32_t)first_FDT_item_no;
    }
    else if(current->file_medium_tag != cn_limit_uint32)
    {
        __DFFSD_read_FDT_item(DBX_flash_tag,current->file_medium_tag,&FDT_item); 
        //parent是cn_limit_uint32,说明current_file对应的文件已经被删除
        if(__pick_little_32bit(FDT_item.parent,0) == cn_limit_uint32)
        {
            //当前文件被删除，取第一个子项
            __DFFSD_read_FDT_item(DBX_flash_tag,first_FDT_item_no,&FDT_item);
            __DFFSD_uasm_FDT_item(DBX_flash_tag,current,&FDT_item);
            current->home_DBX = parent->home_DBX;
            current->file_medium_tag = (ptu32_t)first_FDT_item_no;
        }
        else
        {
          	FDT_item_no = __pick_little_32bit(FDT_item.next,0);
            if(FDT_item_no == first_FDT_item_no)    //current_file已经是最后一个
            {
                m_free(current);
                return NULL;
            }
        	__DFFSD_read_FDT_item(DBX_flash_tag,FDT_item_no,&FDT_item);
            __DFFSD_uasm_FDT_item(DBX_flash_tag,current,&FDT_item);
            current->home_DBX = parent->home_DBX;
            current->file_medium_tag = (ptu32_t)FDT_item_no;
        }
    }
    else
    {
        return NULL;    //当前位置非法
    }
    return current;
}


//----比较名字-----------------------------------------------------------------
//功能: 比较两个文件(目录)名字符串是否相等，因文件系统支持255字符，而flash
//      driver只支持cn_file_name_len个字符，因此当fs_name的长度超过
//      cn_file_name_len时将截尾比较。若截尾后长度仍不相等，直接返回false，若长
//      度相等，则逐字符比较，全等返回true，否则返回false。
//参数: fs_name，由文件系统提供的文件名指针
//      FDT_name，FDT表中读取的文件名指针
//返回: true = 相等，false=不相等
//----------------------------------------------------------------------------
bool_t __DFFSD_cmp_file_name(char *fs_name,char *FDT_name)
{
    uint32_t fs_len,FDT_len;
    //fs_name是已经经过字符串长度合法性检查的指针
    fs_len = strlen(fs_name);
    //FDT_name是已经经过字符串长度合法性检查的指针
    FDT_len = strlen(FDT_name);
    if(fs_len > cn_flash_file_name_limit)
        fs_len = cn_flash_file_name_limit;
    if(fs_len != FDT_len)
        return false;
    fs_len = fs_len > FDT_len? FDT_len:fs_len;
    for(FDT_len = 0; FDT_len < fs_len; FDT_len++)
        if(fs_name[FDT_len] != FDT_name[FDT_len])
            return false;
    return true;
}

//----分配文件缓冲区-----------------------------------------------------------
//功能: 根据文件的打开模式，为文件分配缓冲区。
//参数: DBX_flash_tag，被操作的flash文件柜的存储媒体标签
//      fp，由文件系统提供的文件名指针
//      mode，文件打来模式
//返回: true = 相等，false=不相等
//----------------------------------------------------------------------------
bool_t __DFFSD_allocate_file_buf(struct st_DBX_flash_tag *DBX_flash_tag,
                              struct file_rsc *fp,
                              enum file_open_mode mode)
{
    struct flash_chip *chip;
    uint32_t read_buf_len;
    uint32_t write_buf_len;
    uint8_t *buf;
    uint8_t *buf1;
    chip = DBX_flash_tag->chip;
    read_buf_len = chip->DBX_read_buf_size;
    write_buf_len = chip->DBX_write_buf_size;
    if(fp->attr.bits.folder)     //FDT条目是一个目录
    {
        ring_init(&fp->read_rw_buf,NULL,0);     //目录的读缓冲区设为空
        ring_init(&fp->write_buf,NULL,0);       //目录的写缓冲区设为空
        fp->p_read_buf = NULL;
        fp->p_write_buf = NULL;
        fp->read_ptr = 0;
        fp->write_ptr = 0;
        fp->file_size = 0;
    }else       //是一个文件，需要根据mode分配读写缓冲区
    {
        switch(mode)
        {
            case enum_r_rb:
            {
                buf =(uint8_t*)m_malloc(read_buf_len,0);//本模式只有读缓冲区
                if(buf == NULL)
                    return false;           //缓冲区分配失败，返回错误
                ring_init(&fp->read_rw_buf,buf,read_buf_len);   //初始化读缓冲区
                ring_init(&fp->write_buf,NULL,0);           //初始化写缓冲区为空
                fp->p_read_buf = &fp->read_rw_buf;  //读缓冲区指针赋值
                fp->p_write_buf = NULL;             //写缓冲区指针置空
                fp->read_ptr = 0;                   //读位置置0
                fp->write_ptr = 0;                  //写位置置0
            }break;
            case enum_w_wb:
            {
                buf =(uint8_t*)m_malloc(write_buf_len,0);//本模式只有写缓冲区
                if(buf == NULL)
                    return false;           //缓冲区分配失败，返回错误
                ring_init(&fp->read_rw_buf,NULL,0);         //初始化读缓冲区为空
                ring_init(&fp->write_buf,buf,write_buf_len);//初始化写缓冲区
                fp->p_read_buf = NULL;              //读缓冲区指针置空
                fp->p_write_buf = &fp->write_buf;   //写缓冲区指针赋值
                fp->read_ptr = 0;                   //读位置置0
                fp->write_ptr = 0;                  //写位置置0
                __DFFSD_set_file_size(fp,0);//本模式需要把文件清空
            }break;
            case enum_a_ab:
            {
                buf =(uint8_t*)m_malloc(write_buf_len,0);//本模式只有写缓冲区
                if(buf == NULL)
                    return false;           //缓冲区分配失败，返回错误
                ring_init(&fp->read_rw_buf,NULL,0);         //初始化读缓冲区为空
                ring_init(&fp->write_buf,buf,write_buf_len);//初始化写缓冲区
                fp->p_read_buf = NULL;              //读缓冲区指针置空
                fp->p_write_buf = &fp->write_buf;   //写缓冲区指针赋值
                fp->read_ptr = 0;                   //读位置置0
                fp->write_ptr = fp->file_size;      //写位置指向文件尾
            }break;
            case enum_r_rb_plus:
            {
                buf = (uint8_t*)m_malloc(read_buf_len,0);      //分配读缓冲区
                if(buf == NULL)
                    return false;           //缓冲区分配失败，返回错误
                buf1 = (uint8_t*)m_malloc(write_buf_len,0);    //分配写缓冲区
                if(buf1 == NULL)
                {

                    m_free(buf);
                return false;           //缓冲区分配失败，返回错误
                }
                ring_init(&fp->read_rw_buf,buf,read_buf_len);   //初始化读缓冲区
                ring_init(&fp->write_buf,buf1,write_buf_len);   //初始化写缓冲区
                fp->p_read_buf = &fp->read_rw_buf;      //设置读缓冲区指针
                fp->p_write_buf = &fp->write_buf;       //设置写缓冲区指针
                fp->read_ptr = 0;                       //读位置置0
                fp->write_ptr = 0;                      //写位置置0
            }break;
            case enum_w_wb_plus:
            {
                buf = (uint8_t*)m_malloc(read_buf_len,0);      //分配读缓冲区
                if(buf == NULL)
                    return false;           //缓冲区分配失败，返回错误
                buf1 = (uint8_t*)m_malloc(write_buf_len,0);    //分配写缓冲区
                if(buf1 == NULL)
                {

                    m_free(buf);
                    return false;           //缓冲区分配失败，返回错误
                }
                ring_init(&fp->read_rw_buf,buf,read_buf_len);   //初始化读缓冲区
                ring_init(&fp->write_buf,buf1,write_buf_len);   //初始化写缓冲区
                fp->p_read_buf = &fp->read_rw_buf;      //设置读缓冲区指针
                fp->p_write_buf = &fp->write_buf;       //设置写缓冲区指针
                fp->read_ptr = 0;                       //读位置置0
                fp->write_ptr = 0;                      //写位置置0
                __DFFSD_set_file_size(fp,0);//本模式需要把文件清空
            }break;
            case enum_a_ab_plus:
            {
                buf = (uint8_t*)m_malloc(read_buf_len,0);      //分配读缓冲区
                if(buf == NULL)
                    return false;           //缓冲区分配失败，返回错误
                buf1 = (uint8_t*)m_malloc(write_buf_len,0);    //分配写缓冲区
                if(buf1 == NULL)
                {

                    m_free(buf);
                    return false;           //缓冲区分配失败，返回错误
                }
                ring_init(&fp->read_rw_buf,buf,read_buf_len);   //初始化读缓冲区
                ring_init(&fp->write_buf,buf1,write_buf_len);   //初始化写缓冲区
                fp->p_read_buf = &fp->read_rw_buf;      //设置读缓冲区指针
                fp->p_write_buf = &fp->write_buf;       //设置写缓冲区指针
                fp->read_ptr = 0;                       //读位置置0
                fp->write_ptr = fp->file_size;          //写位置指向文件尾
            }break;
            default:break;
        }
    }
    return true;
}

//----打开（文件/目录）--------------------------------------------------------
//功能：在文件柜中查找并打开文件(目录)，如果是文件，还需要为它分配读写缓冲区。只
//      查父目录的子项，不查孙项及以下
//参数：name，文件（目录）名
//      parent，文件/目录的父目录，
//      result，返回找到的文件的信息
//      mode，文件（目录）打开模式
//返回：cn_fs_open_success,成功打开文件(目录)
//      cn_fs_item_exist,文件(目录)存在但不能打开(一般是模式不兼容)
//      cn_fs_item_inexist,文件(目录)不存在
//      cn_limit_uint32，其他错误
//----------------------------------------------------------------------------
uint32_t DFFSD_open_item(char *name,struct file_rsc *parent,
                       struct file_rsc *result,enum file_open_mode mode)
{
    uint32_t FDT_item_no,FDT_item_eldest;
    struct st_DBX_device_tag *DBX_device_tag;
    struct st_DBX_flash_tag *DBX_flash_tag;
    struct fdt_info FDT_item;
    bool_t found = false;

    if((parent == NULL) || (result == NULL))
        return cn_limit_uint32;
    DBX_device_tag = (struct st_DBX_device_tag *)(parent->home_DBX->private_tag);
    DBX_flash_tag = (struct st_DBX_flash_tag *)DBX_device_tag->DBX_medium_tag;

    FDT_item_no = (uint32_t)parent->file_medium_tag;
    //读取父目录的FDT表项
    __DFFSD_read_FDT_item(DBX_flash_tag,FDT_item_no,&FDT_item);
    if(!(FDT_item.mattr & cn_FS_ATTR_DIRECTORY))
        return cn_limit_uint32;       //父目录实际上是一个文件
    //读取父目录的第一个子目录/文件的FDT表项号
    FDT_item_eldest = __pick_little_32bit(FDT_item.fstart_dson,0);
    if(FDT_item_eldest == cn_limit_uint32)
        return cn_fs_item_inexist;  //parent是一个空目录
    FDT_item_no = FDT_item_eldest;
    do  //循环读目录表，直到读完
    {
        //读取的FDT表项
        __DFFSD_read_FDT_item(DBX_flash_tag,FDT_item_no,&FDT_item);
        FDT_item.name[cn_flash_file_name_limit] = '\0'; //字符串结束符，容错用
        if(__DFFSD_cmp_file_name(name,FDT_item.name))
        {
            found = true;       //找到匹配的文件(目录)
            break;
        }else
        {
        	if(__pick_little_32bit(FDT_item.next,0) ==FDT_item_no)//避免死循环
            	break;
            FDT_item_no = __pick_little_32bit(FDT_item.next,0);
            if(FDT_item_no == cn_limit_uint32)//防止死循环
                break;
        }
    }while(FDT_item_no != FDT_item_eldest);
    if(found)
    {
        result->file_medium_tag = (ptu32_t)FDT_item_no;
        result->file_semp = NULL;
        result->open_mode = mode;
        //从flash中FDT条目的内存镜像FDT_item中提取文件信息
        __DFFSD_uasm_FDT_item(DBX_flash_tag,result,&FDT_item);
        if(__DFFSD_allocate_file_buf(DBX_flash_tag,result,mode))//分配文件缓冲区
        {
            return cn_fs_open_success;
        }else
            return cn_fs_item_exist;
    }else
        return cn_fs_item_inexist;
}

//----查找（文件/目录）--------------------------------------------------------
//功能：在文件柜中查找并打开文件(目录)，如果是文件，还需要为它分配读写缓冲区。
//参数：name，文件（目录）名
//      parent，文件/目录的父目录，
//      result，返回找到的文件的信息
//返回：true=文件/目录存在，并在result中返回文件信息(不分配文件缓冲区)。
//      false = 文件/目录不存在，不修改result。
//----------------------------------------------------------------------------
bool_t DFFSD_lookfor_item(char *name,struct file_rsc *parent,
                         struct file_rsc *result)
{
    uint32_t FDT_item_no,FDT_item_eldest;
    struct st_DBX_device_tag *DBX_device_tag;
    struct st_DBX_flash_tag *DBX_flash_tag;
    struct fdt_info FDT_item;
    bool_t found = false;
    if(parent == NULL)
        return false;
    if(result != NULL)
        rsc_clean_node(&result->file_node);
    DBX_device_tag = (struct st_DBX_device_tag *)(parent->home_DBX->private_tag);
    DBX_flash_tag = (struct st_DBX_flash_tag *)DBX_device_tag->DBX_medium_tag;
    FDT_item_no = (uint32_t)parent->file_medium_tag;
    //读取父目录的FDT表项
    __DFFSD_read_FDT_item(DBX_flash_tag,FDT_item_no,&FDT_item);
    if(!(FDT_item.mattr & cn_FS_ATTR_DIRECTORY))
        return false;       //父目录实际上是一个文件
    //读取父目录的第一个子目录/文件的FDT表项号
    FDT_item_eldest = __pick_little_32bit(FDT_item.fstart_dson,0);
    if(FDT_item_eldest == cn_limit_uint32)
        return false;  //parent是一个空目录
    FDT_item_no = FDT_item_eldest;
    do  //循环读目录表，直到读完
    {
        //读取的FDT表项
        __DFFSD_read_FDT_item(DBX_flash_tag,FDT_item_no,&FDT_item);
        FDT_item.name[cn_flash_file_name_limit] = '\0'; //字符串结束符，容错用
        if(__DFFSD_cmp_file_name(name,FDT_item.name))
        {
            if(result != NULL)
            {
                result->file_medium_tag = (ptu32_t)FDT_item_no;
                result->file_semp = NULL;

            	result->home_DBX = parent->home_DBX;    
                //从flash中FDT条目的内存镜像FDT_item中提取文件信息
                __DFFSD_uasm_FDT_item(DBX_flash_tag,result,&FDT_item);
            }
            found = true;
            break;
        }else
        {
            FDT_item_no = __pick_little_32bit(FDT_item.next,0);
        }
    }while(FDT_item_no != FDT_item_eldest);
    return found;
}

//----关闭文件(目录)-----------------------------------------------------------
//功能: 关闭一个打开的文件(目录)，如果是文件，则释放读写缓冲区，并使文件柜中
//      opened_sum减量，如果是目录，则只是opened_sum减量。本函数只是处理关闭过程
//      与存储介质相关的操作，系统资源链表由file.c模块处理。
//参数：fp，被操作的文件指针
//返回：true=成功，false=失败
//-----------------------------------------------------------------------------
bool_t DFFSD_close_item(struct file_rsc *fp)
{
    if(fp == NULL)
        return false;
    if(fp->attr.bits.folder)
    {
        //是个目录，好像没什么需要处理的
    }else
    {
        if(ring_capacity(&fp->write_buf) != 0)    //查看文件写缓冲区是否空
        {
            DFFSD_flush_file(fp);//写缓冲区数据必须写入
            m_free(ring_get_buf(&fp->write_buf)); //文件写缓冲区非空，释放之
        }
        if(ring_capacity(&fp->read_rw_buf) != 0)     //查看文件读缓冲区是否空
        {
            m_free(ring_get_buf(&fp->read_rw_buf));  //文件读缓冲区非空，释放之
        }
    }
    return true;
}

//----文件（目录）改名---------------------------------------------------------
//功能：修改一个文件（目录）的名字，新名字保存在fp中。fp中的名字可以达到255字
//      符，但只有前cn_flash_file_name_limit字符有效。
//参数：fp，被操作的文件指针
//返回：true=成功，false=失败
//-----------------------------------------------------------------------------
bool_t DFFSD_rename_item(struct file_rsc *fp,char *newname)
{
    struct st_DBX_device_tag *DBX_device_tag;
    struct fdt_info FDT_item;
    struct st_DBX_flash_tag *DBX_flash_tag;
    uint32_t name_len;
    uint32_t FDT_item_no;
    if(fp == NULL)
        return false;

    DBX_device_tag = (struct st_DBX_device_tag *)(fp->home_DBX->private_tag);
    DBX_flash_tag = (struct st_DBX_flash_tag *)DBX_device_tag->DBX_medium_tag;
    FDT_item_no = (uint32_t)fp->file_medium_tag;
    __DFFSD_read_FDT_item(DBX_flash_tag,FDT_item_no,&FDT_item);
    //fp->name是文件系统模块提供的，其名字的长度本来应该是合法的，但是fp作为
    //一个资源结点，是暴露在阳光下的，有可能被误修改，故不能使用strlen。
    name_len = strnlen(fp->name,cn_flash_file_name_limit+1);
    if(name_len <= cn_flash_file_name_limit)  //文件柜名字不超过cn_flash_file_name_limit
        strcpy(FDT_item.name,newname);
    else
    {

        memcpy(FDT_item.name,newname,cn_flash_file_name_limit);
        FDT_item.name[cn_flash_file_name_limit] = '\0';
    }
    __DFFSD_write_FDT_item(DBX_flash_tag,FDT_item_no,&FDT_item);
    return true;
}

//----检测并改正FDT表-----------------------------------------------------------
//功能：如果在创建或删除文件的过程中断电，将可能导致FDT表中正在操作的链表断裂，
//      本函数检测并修复之
//参数：DBX_device_tag，被操作的文件柜指针，通用结构，非flash专用。
//返回：true=无需更改或者成功更改，false=有错误但修复失败
//-----------------------------------------------------------------------------
bool_t __DFFSD_repair_FDT(struct st_DBX_flash_tag *DBX_flash_tag)
{
    return true;
}

//----查询目录表尺寸-----------------------------------------------------------
//功能：查询一个文件柜的目录表尺寸，按字节计算。当应用程序需要读取整个目录表时，
//      必须先用本函数计算目录表尺寸，然后才能分配内存。
//参数：DBX_device_tag，被操作的文件柜指针，通用结构，非flash专用。
//返回：目录表尺寸
//-----------------------------------------------------------------------------
uint32_t DFFSD_check_FDT_size(struct st_DBX_device_tag *DBX_device_tag)
{
    struct st_DBX_flash_tag *DBX_flash_tag;
    struct flash_chip *chip;
    uint32_t result,block_no,block_no_next;
    if(DBX_device_tag == NULL)
        return 0;
    DBX_flash_tag = (struct st_DBX_flash_tag*)DBX_device_tag->DBX_medium_tag;
    chip = DBX_flash_tag->chip;
    block_no_next = DBX_flash_tag->DDR_FDSB;
    result = 0;
    do
    {
        block_no = block_no_next;

       	if(DBX_flash_tag->DDR_MAT[block_no].attr == cn_FDT_block)
        	result ++;
        block_no_next = DBX_flash_tag->DDR_MAT[block_no].next;
    }while(block_no != block_no_next);
    return result * chip->block_size;
}

//----读目录表-----------------------------------------------------------------
//功能：读出一个文件柜的整个目录表，按原始数据读出，不做解释
//参数：DBX_device_tag，被操作的文件柜指针，通用结构，非flash专用。
//      buf，保存读取的数据的缓冲区指针，用户调用前必须分配号空间
//返回：无
//-----------------------------------------------------------------------------
void DFFSD_read_FDT(struct st_DBX_device_tag *DBX_device_tag,uint8_t *buf)
{
    struct st_DBX_flash_tag *DBX_flash_tag;
    struct flash_chip *chip;
    uint32_t block_no,block_no_next;
    uint32_t completed = 0;
    if((DBX_device_tag == NULL) || (buf == NULL))
        return;
    DBX_flash_tag = (struct st_DBX_flash_tag*)DBX_device_tag->DBX_medium_tag;
    chip = DBX_flash_tag->chip;
    block_no_next = DBX_flash_tag->DDR_FDSB;
    do
    {
        block_no = block_no_next;
        chip->read_data_with_ecc(block_no + DBX_flash_tag->start_block,0,
                                 buf+completed,chip->block_size);
        block_no_next = DBX_flash_tag->DDR_MAT[block_no].next;
        completed += chip->block_size;
    }while(block_no != block_no_next);
    return ;
}

//----查询文件柜参数-----------------------------------------------------------
//功能: 返回文件柜总空间，有效空间，空闲空间的字节数
//参数:
//返回: 无
//-----------------------------------------------------------------------------
void DFFSD_check_DBX(struct st_DBX_device_tag *DBX_device_tag,
                     sint64_t *sum_size,sint64_t *valid_size,sint64_t *free_size)
{
    struct st_DBX_flash_tag *DBX_flash_tag;
    struct flash_chip *chip;
    if(DBX_device_tag == NULL)
    {
        if(sum_size != NULL)
            *sum_size = 0;
        if(valid_size != NULL)
            *valid_size = 0;
        if(free_size != NULL)
            *free_size = 0;
    }else
    {
        DBX_flash_tag =(struct st_DBX_flash_tag*)DBX_device_tag->DBX_medium_tag;
        chip = DBX_flash_tag->chip;
        if(sum_size != NULL)
            *sum_size = DBX_flash_tag->block_sum * chip->block_size;
        if(valid_size != NULL)
            *valid_size = DBX_flash_tag->valid_sum * chip->block_size;
        if(free_size != NULL)
            *free_size = DBX_flash_tag->free_sum * chip->block_size;
    }
}
//----填充DBX_device_tag-------------------------------------------------------
//功能: 填充DBX_device_tag
//参数：DBX_device_tag，被操作的文件柜指针，通用结构，非flash专用。
//      name，文件柜名
//返回: 无
//-----------------------------------------------------------------------------
void __DFFSD_fill_DBX_device_tag(struct st_DBX_device_tag *DBX_device_tag,
                                 char *name)
{
    DBX_device_tag->format = DFFSD_format_DBX;
    DBX_device_tag->write = DFFSD_write_file;
    DBX_device_tag->read = DFFSD_read_file;
    DBX_device_tag->flush = DFFSD_flush_file;
    DBX_device_tag->query_file_stocks = DFFSD_query_file_stocks;
    DBX_device_tag->query_file_cubage = DFFSD_query_file_cubage;
    DBX_device_tag->check_DBX = DFFSD_check_DBX;
    DBX_device_tag->set_file_size = __DFFSD_set_file_size;
    DBX_device_tag->seek_file = DFFSD_seek_file;
    DBX_device_tag->create_item = DFFSD_create_file;
//    DBX_device_tag->move_file= DFFSD_move_file; 
    DBX_device_tag->remove_item = DFFSD_remove_file;
    DBX_device_tag->open_item = DFFSD_open_item;
    DBX_device_tag->close_item = DFFSD_close_item;
    DBX_device_tag->lookfor_item = DFFSD_lookfor_item;
    DBX_device_tag->rename_item = DFFSD_rename_item;
	DBX_device_tag->item_traversal_son = DFFSD_item_traversal_son;
    DBX_device_tag->check_fdt_size = DFFSD_check_FDT_size;
    DBX_device_tag->read_fdt = DFFSD_read_FDT;
    DBX_device_tag->check_folder = DFFSD_check_folder;
    DBX_device_tag->opened_sum = 0;
    //name肯定是个长度合法的字符串
    if(strlen(name) <= cn_flash_DBX_name_limit)
        strcpy(DBX_device_tag->name,name);  //读文件柜名字
    else
    {
        memcpy(DBX_device_tag->name,name,cn_flash_DBX_name_limit);
        DBX_device_tag->name[cn_flash_DBX_name_limit] = '\0';
    }
}

//----DFFSD模块初始化----------------------------------------------------------
//功能: 就是在系统资源链表中添加flash芯片根结点
//参数: 无
//返回: 1
//-----------------------------------------------------------------------------
ptu32_t module_init_DFFSD(ptu32_t para)
{
    //在资源链表中建立一个根结点，系统中所有芯片都挂在这个结点下
    rsc_add_tree_root(&tg_flash_chip_root_rsc,sizeof(struct  rsc_node),
                        "flash chip");
    return 1;
}

//----扫描芯片-----------------------------------------------------------------
//功能: 启动时扫描芯片，识别芯片中的文件柜，并把文件柜加入到系统设备链表
//      中成为“fs”设备的子设备。
//参数: chip，被扫描的芯片
//      start_block_no，扫描起始块号，前面的块是保留块
//返回: 无
//-----------------------------------------------------------------------------
void __DFFSD_scan_chip(struct flash_chip *chip,uint32_t start_block_no)
{
    uint32_t loop,table_size,DDR_blocks,restored_block;
    djy_handle_t fs_handle_right = NULL;
    struct st_DBX_flash_tag *DBX_flash_tag = NULL;
    struct st_DBX_device_tag *DBX_device_tag = NULL;
    struct DBX_semp_para semp_dbx;
    struct MDR_temp_record MDR_record;
    struct fdt_info FDT_item;

    chip->valid_sum = chip->block_sum - start_block_no;
    if(! __DFFSD_read_MDR(chip,&MDR_record))
    {//系统第一次启动，肯定没有建立文件柜。故加载的时候，把DFFSD_init_MDR放在这步初始化
        DFFSD_init_MDR(chip,start_block_no); //未初始化的芯片按默认配置初始化
        return ;
    }
    //打开文件系统设备的右手接口
    if((fs_handle_right = dev_open_right("fs",0)) == NULL)
        return ;
    for(loop=0; loop < cn_DBX_sum_max; loop++)   //逐个遍历文件柜
    {
        DBX_device_tag = NULL;
        DBX_flash_tag = NULL;
        if(!chip->MDR.DBX_created[loop])
            continue;
        //分配文件柜设备专用数据结构内存，该结构将嵌入到泛设备结构中
        DBX_device_tag = (struct st_DBX_device_tag*)
                    m_malloc_gbl(sizeof(struct st_DBX_device_tag),0);
        if(DBX_device_tag == NULL)
            goto goto_exit_install_chip;
        //分配flash文件系统专用数据结构内存，该结构将嵌入到文件柜设备中
        DBX_flash_tag = (struct st_DBX_flash_tag*)
                    m_malloc_gbl(sizeof(struct st_DBX_flash_tag),0);
        if(DBX_flash_tag == NULL)
            goto goto_exit_install_chip;
        memset(DBX_flash_tag,0,sizeof(struct st_DBX_flash_tag));
        memset(DBX_device_tag,0,sizeof(struct st_DBX_device_tag));
        DBX_flash_tag->ART_position = NULL;
        DBX_flash_tag->ART_block_no = NULL;
        DBX_flash_tag->ART_times = NULL;
        DBX_flash_tag->writed_DDR_main = NULL;
        DBX_flash_tag->writed_DDR_bak = NULL;
        DBX_flash_tag->DDR_DBL = NULL;
        DBX_flash_tag->DDR_MAT = NULL;
        DBX_flash_tag->chip = chip;     //flash文件柜存储器所属芯片
        DBX_flash_tag->DBX_no = loop;   //本文件柜在芯片中的序号
        DBX_flash_tag->start_block = MDR_record.start_blocks[loop];
        DBX_flash_tag->PCRB_no = MDR_record.PCRB_block_no[loop]
                                    -DBX_flash_tag->start_block;
        DBX_flash_tag->DDR_main = MDR_record.DDR_main_block_no[loop]
                                    -DBX_flash_tag->start_block;
        DBX_flash_tag->DDR_bak = MDR_record.DDR_bak_block_no[loop]
                                    -DBX_flash_tag->start_block;
        DBX_flash_tag->block_sum = MDR_record.blocks_sum[loop];
        chip->valid_sum -= DBX_flash_tag->block_sum;

        //填充函数指针和文件柜名
        __DFFSD_fill_DBX_device_tag(DBX_device_tag,MDR_record.name[loop]);
        //文件柜设备的存储媒体指向flash文件柜存储器
        DBX_device_tag->DBX_medium_tag = (ptu32_t)DBX_flash_tag;

        DBX_device_tag->formatted = MDR_record.formatted[loop];
        //4+4+2=位置表4字节+块号表4字节+次数表2字节
        table_size = DBX_flash_tag->block_sum * (4+4+2);
        //为磨损位置表分配内存
        DBX_flash_tag->ART_position = (uint32_t*)m_malloc_gbl(table_size,0);
        if(DBX_flash_tag->ART_position == NULL)
            goto goto_exit_install_chip;
        //为磨损块号表分配内存
        DBX_flash_tag->ART_block_no = DBX_flash_tag->ART_position
                                      + DBX_flash_tag->block_sum;
        //为磨损次数表分配内存
        DBX_flash_tag->ART_times = (uint16_t *)(DBX_flash_tag->ART_block_no
                                      + DBX_flash_tag->block_sum);
        //计算DDR表尺寸
        DBX_flash_tag->DDR_size = __DFFSD_calculate_DDR_size(DBX_flash_tag);
        //1份DDR表占用的块数
        DDR_blocks = (DBX_flash_tag->DDR_size
                            +chip->block_size-1)/chip->block_size;
        //申请两份DBL表所需内存
        DBX_flash_tag->DDR_DBL =(uint32_t*)m_malloc_gbl(DDR_blocks*8,0);
        if(DBX_flash_tag->DDR_DBL == NULL)
            goto goto_exit_install_chip;
        //申请MAT表所需内存
        DBX_flash_tag->DDR_MAT = (struct MAT_table *)m_malloc_gbl(
                           DBX_flash_tag->block_sum*sizeof(struct MAT_table),0);
        if(DBX_flash_tag->DDR_MAT == NULL)
            goto goto_exit_install_chip;
        //两份写入标志
        DBX_flash_tag->writed_DDR_main = (bool_t*)m_malloc_gbl(
                                            DDR_blocks*sizeof(bool_t)*2,0);
        if(DBX_flash_tag->writed_DDR_main == NULL)
            goto goto_exit_install_chip;
        DBX_flash_tag->writed_DDR_bak=DBX_flash_tag->writed_DDR_main+DDR_blocks;

        if(DBX_device_tag->formatted) //检查文件柜格式化否
        {
            //从意外掉电中恢复数据，接下来要读取DDR,DDR表有被掉电恢复块保护的可能。
            //不理会不可恢复的情况，因为即使有文件被破坏，其他文件仍然可用，如果
            //DDR表被破坏，将在__read_DDR函数中被检出。
            chip->restore_PCRB(DBX_flash_tag->PCRB_no+DBX_flash_tag->start_block,
                                &restored_block);
            if(__DFFSD_scan_DDR(DBX_flash_tag))
            {
                __DFFSD_sort_ART(DBX_flash_tag);
                __DFFSD_read_FDT_item(DBX_flash_tag,0,&FDT_item);
                __DFFSD_uasm_root_folder(DBX_flash_tag,&FDT_item);
            }else    //DDR表出错，文件柜需要重新格式化才能使用
                DBX_device_tag->formatted = false;
            if(restored_block < chip->block_sum)
            {
                if(DBX_flash_tag->DDR_MAT[restored_block].attr == cn_FDT_block)
                    __DFFSD_repair_FDT(DBX_flash_tag);
            }
        }
        semp_dbx.left = chip->left_semaphore;   //芯片的左手信号量
        semp_dbx.right = chip->right_semaphore; //芯片的右手信号量
        if(dev_ctrl_right(fs_handle_right,enum_fs_add_DBX,
                            (ptu32_t)DBX_device_tag,(ptu32_t)&semp_dbx)
                            == enum_fs_no_error)
        {
            continue;   //不释放文件柜设备相关内存，直接进入下一循环
        }else
        {
            goto goto_exit_install_chip;
        }

        //如果已经格式化了，给根目录的file_medium_tag分配内容
        if(DBX_device_tag->formatted)
        {
            DBX_device_tag->opened_root->file_medium_tag = (ptu32_t)0;
        }        
    }
    //这是正常出口
    dev_close_right(fs_handle_right);     //关闭文件系统设备
    return ;
goto_exit_install_chip:
    dev_close_right(fs_handle_right);     //关闭文件系统设备
    if(DBX_flash_tag !=NULL)
    {
        if(DBX_flash_tag->writed_DDR_main != NULL)
            m_free(DBX_flash_tag->writed_DDR_main);
        if(DBX_flash_tag->DDR_MAT != NULL)
            m_free(DBX_flash_tag->DDR_MAT);
        if(DBX_flash_tag->DDR_DBL != NULL)
            m_free(DBX_flash_tag->DDR_DBL);
        if(DBX_flash_tag->ART_position != NULL)
            m_free(DBX_flash_tag->ART_position);
        m_free(DBX_flash_tag);
    }
    if(DBX_device_tag !=NULL)   m_free(DBX_device_tag);
    return ;
}

//----安装一个芯片------------------------------------------------------------
//功能: 把芯片中已经初始化的文件柜登记到文件系统中，该文件柜可能未格式化
//参数: chip，待安装的芯片
//      rev_blocks，该芯片保留块数量
//返回: 建立的文件柜数量，若返回0则表示芯片尚未初始化
//----------------------------------------------------------------------------
bool_t DFFSD_install_chip(struct flash_chip *chip,char *name,uint32_t rev_blocks)
{
#if cn_MDR_size < cn_MDR_CDR_size + cn_MDR_DBX_size +cn_DBX_sum_max*3*8*2+8
    return false;  //如条件成立，MDR滚动部分容量小于2组条目，没有意义。
#endif

    if(chip == NULL)
        return false;
    chip->left_semaphore = semp_create(1,1,name);
    if(chip->left_semaphore == NULL)
        return false;
    if((chip->right_semaphore = semp_create(1,1,name))==NULL)
    {
        semp_delete(chip->left_semaphore);
        return false;
    }
    chip->block_buf = m_malloc_gbl(chip->block_size,0);  //分配块缓冲区内存
    if(chip->block_buf == NULL)
    {
        semp_delete(chip->right_semaphore);
        semp_delete(chip->left_semaphore);
        return false;
    }
    //把新flash芯片加入到flash芯片根资源结点下，排在队列尾
    rsc_add_son(&tg_flash_chip_root_rsc,&chip->chip_node,
                        sizeof(struct flash_chip),name);
    __DFFSD_scan_chip(chip,rev_blocks);
    return true;
}

bool_t get_file_block_no(char *fullname)
{
	struct file_rsc *fp;
    struct st_DBX_device_tag *DBX_device_tag;
    struct st_DBX_flash_tag *DBX_flash_tag;
    struct fdt_info FDT_item;
	uint32_t loop,next_block;
    
	fp = djyfs_fopen(fullname,"r");
    if(fp == NULL)
        return 0;
    DBX_device_tag = (struct st_DBX_device_tag *)(fp->home_DBX->private_tag);
    DBX_flash_tag = (struct st_DBX_flash_tag*)DBX_device_tag->DBX_medium_tag;
    __DFFSD_read_FDT_item(DBX_flash_tag,
                          (uint32_t)fp->file_medium_tag,&FDT_item);
    //是文件，说明当前工作路径操作错误
    if(FDT_item.mattr & cn_FS_ATTR_DIRECTORY)
        return 0;  
	loop = __pick_little_32bit(FDT_item.fstart_dson,0);
	djy_printf("\n0x%8x", loop);
    //djy_printf(fullname);
    //djy_printf("中的文件所占用的块号:\r\n顺序是:");
	do
    {
        next_block = loop;
        loop = DBX_flash_tag->DDR_MAT[next_block].next;
        djy_printf(" 0x%8x", loop);
    }while(loop != next_block);
	djy_printf("\r\n");
	djyfs_fclose(fp);
	return true;    
}

//extern const u8 cs_gb2312_song_16[];
 void test_flash_file( void )
{
    struct flash_chip *chip;
    djyfs_file *fp,*fp1,*fp3,*fp4,*zk;
    struct rsc_node *chip_tree;
    u32  result = 0;
#if 1
    chip_tree = rsc_search_tree("flash chip");
    chip = (struct flash_chip *)rsc_get_son(chip_tree);
    result = DFFSD_create_DBX(chip,1000,"sys");
    djyfs_format(1,0,"sys");

    fp = djyfs_fopen("sys:\\test.dbx","w+");
    fp1 =djyfs_fopen("sys:\\test2.dbx","w+");
    fp3 =djyfs_fopen("sys:\\folder1\\test3.dbx","w+");
    fp4 =djyfs_fopen("sys:\\folder1\\test4.dbx","w+");
    zk = djyfs_fopen("sys:\\gb2312_1616","w+");
//    djyfs_fwrite(cs_gb2312_song_16,282016,1,zk);
#endif
}

#endif //#if ((cfg_djyfs == 1) && (cfg_run_mode != cn_mode_tiny) && (cfg_djyfs_flash == 1))
