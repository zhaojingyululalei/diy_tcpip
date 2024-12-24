#include "package.h"
#include "mmpool.h"
#include "threadpool.h"

static uint8_t pkg_buf[(sizeof(list_node_t) + sizeof(pkg_t)) * PKG_LIMIT];
static uint8_t pkg_databuf[(sizeof(list_node_t) + sizeof(pkg_dblk_t)) * PKG_DATA_BLK_LIMIT];
/**
 * 所有线程中，只有工作线程一个线程，会对数据包进行读写，其他线程仅仅是搬运，不用锁
 */
// static lock_t pkg_locker;
static mempool_t pkg_pool;
static mempool_t pkg_datapool;

int package_pool_init(void)
{
    int ret;
    dbg_info("package pool init ing ...\n");

    // ret = lock_init(&pkg_locker);
    // if (ret < 0)
    // {
    //     return ret;
    // }
    ret = mempool_init(&pkg_pool, pkg_buf, PKG_LIMIT, sizeof(pkg_t));
    if (ret < 0)
    {
        return ret;
    };
    ret = mempool_init(&pkg_datapool, pkg_databuf, PKG_DATA_BLK_LIMIT, sizeof(pkg_dblk_t));
    if (ret < 0)
    {
        return ret;
    };
    dbg_info("package pool init finsh!...\n");
    return -1;
}
int package_pool_destory(void)
{
    int ret;
    dbg_info("package pool destory...\n");
    // ret = lock_destory(&pkg_locker);
    // if (ret < 0)
    // {
    //     return ret;
    // };
    mempool_destroy(&pkg_pool);
    mempool_destroy(&pkg_datapool);
    dbg_info("package pool destory finish! ...\n");
    return 0;
}

static void package_init(pkg_t *pkg, int size)
{
    ////lock(&pkg_locker);
    pkg->total = size;
    list_init(&pkg->pkgdb_list);
    pkg->node.next = NULL;
    pkg->node.pre = NULL;
    ////un//lock(&pkg_locker);
}
static void package_destory(pkg_t *package)
{
    ////lock(&pkg_locker);
    package->total = 0;
    package->node.next = NULL;
    package->node.pre = NULL;
    list_destory(&package->pkgdb_list);
    ////un//lock(&pkg_locker);
}
#include <string.h>
static void package_datablk_destory(pkg_dblk_t *blk)
{
    ////lock(&pkg_locker);
    blk->node.next = NULL;
    blk->node.pre = NULL;
    blk->offset = 0;
    blk->size = 0;
    memset(blk->data, 0, PKG_DATA_BLK_SIZE);
    ////un//lock(&pkg_locker);
}

int package_collect(pkg_t *package)
{
    int ret;
    if (!package)
    {
        return -3;
    }
    // lock(&pkg_locker);
    //  先回收数据块
    while (list_count(&package->pkgdb_list) > 0)
    {

        list_node_t *dnode = list_remove_first(&package->pkgdb_list);
        pkg_dblk_t *blk = list_node_parent(dnode, pkg_dblk_t, node);
        package_datablk_destory(blk);
        ret = mempool_free_blk(&pkg_datapool, blk);
        if (ret != 0)
        {
            // un//lock(&pkg_locker);
            return ret;
        }
    }
    package_destory(package);
    ret = mempool_free_blk(&pkg_pool, package);
    // un//lock(&pkg_locker);
    return 0;
}

pkg_t *package_alloc(int size)
{
    int ret;
    pkg_t *package = mempool_alloc_blk(&pkg_pool, -1);
    memset(package, 0, sizeof(pkg_t));
    if (package == NULL)
    {
        return NULL;
    }
    // lock(&pkg_locker);
    package_init(package, size);
    int remain_size = size;
    while (remain_size > 0)
    {
        pkg_dblk_t *dblk = mempool_alloc_blk(&pkg_datapool, -1);
        memset(dblk, 0, sizeof(pkg_dblk_t));
        if (dblk == NULL)
        {
            ret = package_collect(package);
            if (ret != 0)
                dbg_error("not ok here\n");
            // un//lock(&pkg_locker);
            return NULL;
        }
        dblk->size = PKG_DATA_BLK_SIZE;
        list_insert_last(&package->pkgdb_list, &dblk->node);
        package->curblk = dblk;
        if (remain_size < PKG_DATA_BLK_SIZE)
        {
            package->inner_offset = remain_size;
            break;
        }
        remain_size -= PKG_DATA_BLK_SIZE;
    }
    // un//lock(&pkg_locker);
    package->pos = size;
    package_lseek(package, 0);
    return package;
}

int package_expand_front(pkg_t *package, int ex_size)
{
    // lock(&pkg_locker);
    package->total += ex_size;
    list_t *fnode = list_first(&package->pkgdb_list);
    pkg_dblk_t *fdata = list_node_parent(fnode, pkg_dblk_t, node);
    int front_leave = fdata->offset;
    if (front_leave >= ex_size)
    {
        fdata->offset -= ex_size;
        fdata->size += ex_size;
    }
    else
    {
        int oringal_offset = fdata->offset;
        int oringal_size = fdata->size;
        fdata->offset = 0;
        fdata->size = PKG_DATA_BLK_SIZE;
        int new_size = ex_size - front_leave;
        pkg_dblk_t *new_blk = mempool_alloc_blk(&pkg_datapool, -1);
        if (new_blk == NULL)
        {
            dbg_warning("pkg data blk alloc failed\n");
            package->total -= ex_size;
            fdata->offset = oringal_offset;
            fdata->size = oringal_size;
            // un//lock(&pkg_locker);
            return -2;
        }
        new_blk->size = new_size;
        new_blk->offset = PKG_DATA_BLK_SIZE - new_size;
        list_insert_first(&package->pkgdb_list, &new_blk->node);
    }
    // un//lock(&pkg_locker);
    return 0;
}

int package_expand_last(pkg_t *package, int ex_size)
{
    // lock(&pkg_locker);
    package->total += ex_size;
    list_t *lnode = list_last(&package->pkgdb_list);
    pkg_dblk_t *lblk = list_node_parent(lnode, pkg_dblk_t, node);
    int last_leave = PKG_DATA_BLK_SIZE - (lblk->offset + lblk->size);
    if (last_leave >= ex_size)
    {
        lblk->size += ex_size;
    }
    else
    {
        int original_offset = lblk->offset;
        int original_size = lblk->size;
        lblk->size = PKG_DATA_BLK_SIZE;
        lblk->offset = 0;
        pkg_dblk_t *blk = mempool_alloc_blk(&pkg_datapool, -1);
        if (blk == NULL)
        {
            dbg_warning("pkg data blk alloc failed\n");
            package->total -= ex_size;
            lblk->size = original_size;
            lblk->offset = original_offset;
            // un//lock(&pkg_locker);
            return -2;
        }
        int new_size = ex_size - last_leave;
        blk->size = new_size;
        blk->offset = 0;
        list_insert_last(&package->pkgdb_list, &blk->node);
    }
    // un//lock(&pkg_locker);
    return 0;
}

int package_expand_front_align(pkg_t *package, int ex_size)
{
    // lock(&pkg_locker);
    package->total += ex_size;
    list_node_t *fnode = list_first(&package->pkgdb_list);
    pkg_dblk_t *fdata = list_node_parent(fnode, pkg_dblk_t, node);
    int front_leave = fdata->offset;
    // 前面空间够
    if (front_leave >= ex_size)
    {
        fdata->offset -= ex_size;
        fdata->size += ex_size;
    }
    else // 前面空间不够
    {
        pkg_dblk_t *new_blk = mempool_alloc_blk(&pkg_datapool, -1);
        memset(new_blk, 0, sizeof(pkg_dblk_t));
        if (new_blk == NULL)
        {
            package->total -= ex_size;
            // un//lock(&pkg_locker);
            return -2;
        }
        new_blk->offset = PKG_DATA_BLK_SIZE - ex_size;
        new_blk->size = ex_size;
        list_insert_first(&package->pkgdb_list, &new_blk->node);
    }
    // un//lock(&pkg_locker);
    return 0;
}

/*show*/
int package_show_pool_info(void)
{
    dbg_info("package pool info.....................\n");
    dbg_info("package pool free pkg cnt is %d\n", mempool_freeblk_cnt(&pkg_pool));
    dbg_info("pkg data pool free blk cnt is %d\n", mempool_freeblk_cnt(&pkg_datapool));
    dbg_info("\n");
    return 0;
}
int package_show_info(pkg_t *package)
{
    if (package == NULL)
    {
        return -1;
    }
    int count = 0, i = 1;
    // lock(&pkg_locker);
    list_node_t *cur = &package->node;
    while (cur)
    {
        count++;
        cur = cur->next;
    }
    dbg_info("package info ......................\n");
    dbg_info("pkg total size is: %d\n", package->total);
    dbg_info("pkg brother link list count(include itself): %d\n", count);
    list_node_t *fnode = list_first(&package->pkgdb_list);
    pkg_dblk_t *curblk = list_node_parent(fnode, pkg_dblk_t, node);
    while (curblk)
    {
        dbg_info("blk[%d].size=%d, .offset=%d\n",
                 i++, curblk->size, curblk->offset);
        curblk = package_get_next_datablk(curblk);
    }
    dbg_info("pkg sum data block cnt is %d\n", i - 1);
    // un//lock(&pkg_locker);
    dbg_info("\n");
    return 0;
}
/**
 * 给数据包添加包头
 */
int package_add_headspace(pkg_t *package, int h_size)
{
    return package_expand_front_align(package, h_size);
}
pkg_dblk_t *package_get_next_datablk(pkg_dblk_t *cur_blk)
{
    // lock(&pkg_locker);
    list_node_t *next_node = cur_blk->node.next;
    if (next_node == NULL)
    {
        // un//lock(&pkg_locker);
        return NULL;
    }
    pkg_dblk_t *next_blk = list_node_parent(next_node, pkg_dblk_t, node);
    // un//lock(&pkg_locker);
    return next_blk;
}
pkg_dblk_t *package_get_pre_datablk(pkg_dblk_t *cur_blk)
{
    // lock(&pkg_locker);
    list_node_t *pre_node = cur_blk->node.pre;
    if (pre_node == NULL)
    {
        // un//lock(&pkg_locker);
        return NULL;
    }
    pkg_dblk_t *pre_blk = list_node_parent(pre_node, pkg_dblk_t, node);
    // un//lock(&pkg_locker);
    return pre_blk;
}

int package_remove_one_blk(pkg_t *package, pkg_dblk_t *delblk)
{
    // lock(&pkg_locker);
    if (package == NULL || delblk == NULL)
    {
        // un//lock(&pkg_locker);
        return -1;
    }
    list_t *list = &package->pkgdb_list;
    list_node_t *delnode = &delblk->node;
    list_remove(list, delnode);
    mempool_free_blk(&pkg_datapool, delblk);
    // un//lock(&pkg_locker);
    return 0;
}

pkg_dblk_t *package_get_first_datablk(pkg_t *package)
{
    // lock(&pkg_locker);
    list_t *list = &package->pkgdb_list;
    list_node_t *fnode = list_first(list);
    if (fnode == NULL)
    {
        // un//lock(&pkg_locker);
        dbg_error("pkg has no blk\n");
        return NULL;
    }
    else
    {
        // un//lock(&pkg_locker);
        return list_node_parent(fnode, pkg_dblk_t, node);
    }
    // un//lock(&pkg_locker);
    return NULL;
}
pkg_dblk_t *package_get_last_datablk(pkg_t *package)
{
    // lock(&pkg_locker);
    list_t *list = &package->pkgdb_list;
    list_node_t *lnode = list_last(list);
    if (lnode == NULL)
    {
        // un//lock(&pkg_locker);
        dbg_error("pkg has no blk\n");
        return NULL;
    }
    else
    {
        // un//lock(&pkg_locker);
        return list_node_parent(lnode, pkg_dblk_t, node);
    }
    // un//lock(&pkg_locker);
    return NULL;
}

int package_shrank_front(pkg_t *package, int sh_size)
{
    int ret = 0;
    // lock(&pkg_locker);
    if (sh_size > package->total)
    {
        // un//lock(&pkg_locker);
        dbg_error("sh_size > pkg size");
        return -1;
    }

    pkg_dblk_t *fblk = package_get_first_datablk(package);
    if (fblk->size > sh_size)
    {
        fblk->size -= sh_size;
        fblk->offset += sh_size;
    }
    else if (fblk->size == sh_size)
    {
        ret = package_remove_one_blk(package, fblk);
        if (ret != 0)
            dbg_warning("package remove one blk fail...\n");
    }
    else
    {
        int leave_sh_size = sh_size - fblk->size;
        int rmv_count = leave_sh_size / PKG_DATA_BLK_SIZE + 1; // 移除剩余块+移除当前块
        leave_sh_size = leave_sh_size % PKG_DATA_BLK_SIZE;
        for (int i = 0; i < rmv_count; ++i)
        {
            ret = package_remove_one_blk(package, fblk);
            if (ret != 0)
                dbg_warning("package remove one blk fail...\n");
            fblk = package_get_first_datablk(package);
        }

        fblk->size -= leave_sh_size;
        fblk->offset += leave_sh_size;
    }
    // un//lock(&pkg_locker);
    package->total -= sh_size;
    ret = package_lseek(package, 0);
    if (ret < 0)
    {
        dbg_error("pkg lseek fail\r\n");
    }
    return ret;
}

int package_shrank_last(pkg_t *package, int sh_size)
{
    int ret = 0;
    // lock(&pkg_locker);
    if (sh_size > package->total)
    {
        // un//lock(&pkg_locker);
        dbg_error("sh_size > pkg size");
        return -1;
    }

    pkg_dblk_t *lblk = package_get_last_datablk(package);
    if (lblk->size > sh_size)
    {
        lblk->size -= sh_size;
    }
    else if (lblk->size == sh_size)
    {
        ret = package_remove_one_blk(package, lblk);
        if (ret != 0)
            dbg_warning("package remove one blk fail...\n");
    }
    else
    {
        int leave_sh_size = sh_size - lblk->size;
        int rmv_count = leave_sh_size / PKG_DATA_BLK_SIZE + 1; // 移除剩余块+移除当前块
        leave_sh_size = leave_sh_size % PKG_DATA_BLK_SIZE;
        for (int i = 0; i < rmv_count; ++i)
        {
            ret = package_remove_one_blk(package, lblk);
            if (ret != 0)
                dbg_warning("package remove one blk fail...\n");
            lblk = package_get_last_datablk(package);
        }

        lblk->size -= leave_sh_size;
    }
    // un//lock(&pkg_locker);
    package->total -= sh_size;
    ret = package_lseek(package, 0);
    if (ret < 0)
    {
        dbg_error("pkg lseek fail\r\n");
    }
    return ret;
}

/**
 * 将所有数据包头整合到一个数据块中
 */
int package_integrate_header(pkg_t *package, int all_hsize)
{
    if (package == NULL || all_hsize <= 0 || all_hsize > package->total)
    {
        dbg_error("Invalid package or header size!\n");
        return -1; // 参数校验
    }

    if (all_hsize > PKG_DATA_BLK_SIZE)
    {
        dbg_error("Header size greater than data block size!\n");
        return -1;
    }

    // lock(&pkg_locker);

    list_node_t *fnode = list_first(&package->pkgdb_list);
    if (fnode == NULL)
    {
        // un//lock(&pkg_locker);
        dbg_error("Package data block list is empty!\n");
        return -1; // 空包检查
    }

    pkg_dblk_t *fblk = list_node_parent(fnode, pkg_dblk_t, node);

    // 如果第一个数据块足够容纳所有头数据，无需整合
    if (fblk->size >= all_hsize)
    {
        // un//lock(&pkg_locker);
        return 0;
    }

    uint8_t *dest = fblk->data;               // 目标位置
    uint8_t *src = fblk->data + fblk->offset; // 当前块的起始位置
    int copied = fblk->size;                  // 已复制的数据大小

    if (copied > 0)
    {
        memmove(dest, src, copied); // 利用 memmove 处理重叠区域
    }

    pkg_dblk_t *next = package_get_next_datablk(fblk);
    if (next == NULL)
    {
        // un//lock(&pkg_locker);
        dbg_error("Header size may be incorrect, not enough data blocks!\n");
        return -1;
    }

    // 从下一个数据块中补充剩余的头数据
    int remaining = all_hsize - copied; // 需要补充的数据量
    src = next->data + next->offset;

    if (remaining > next->size)
    {
        remaining = next->size; // 防止越界
    }

    memcpy(dest + copied, src, remaining); // 将下一块数据复制到目标位置

    next->offset += remaining; // 更新下一个块的偏移和大小
    next->size -= remaining;
    fblk->size = all_hsize; // 更新第一个块的大小
    fblk->offset = 0;

    // 如果下一个数据块已空，移除它
    if (next->size == 0)
    {
        package_remove_one_blk(package, next);
    }

    // un//lock(&pkg_locker);

    return 0;
}

int package_integrate_two_continue_blk(pkg_t *package, pkg_dblk_t *blk)
{
    if (package == NULL || blk == NULL)
    {
        dbg_error("Invalid package or block!\n");
        return 0;
    }

    // lock(&pkg_locker);

    pkg_dblk_t *next = package_get_next_datablk(blk);
    if (next == NULL)
    {
        // un//lock(&pkg_locker);
        return 0;
    }

    int both_size = blk->size + next->size;
    if (both_size > PKG_DATA_BLK_SIZE)
    {
        // un//lock(&pkg_locker);
        return 0;
    }

    // 使用 memcpy 替代手动拷贝
    memmove(blk->data, blk->data + blk->offset, blk->size);
    blk->offset = 0;       // 重置偏移
    blk->size = both_size; // 更新总大小

    memcpy(blk->data + blk->size, next->data + next->offset, next->size);

    // 移除 next 数据块
    if (package_remove_one_blk(package, next) != 0)
    {
        dbg_warning("Failed to remove the next block\n");
    }

    // un//lock(&pkg_locker);
    return 1;
}

int package_join(pkg_t *from, pkg_t *to)
{
    if (from == NULL || to == NULL)
    {
        return -1;
    }

    // lock(&pkg_locker);

    // 合并两包的总大小
    to->total += from->total;

    // 合并链表
    list_t *list_f = &from->pkgdb_list;
    list_t *list_to = &to->pkgdb_list;
    list_join(list_f, list_to);

    // 清理 from 的内容
    if (package_collect(from) != 0)
    {
        // un//lock(&pkg_locker);
        dbg_warning("Failed to clean the source package\n");
        return -1;
    }

    // 整合目标包的数据块
    pkg_dblk_t *curblk = package_get_first_datablk(to);
    while (curblk)
    {
        package_integrate_two_continue_blk(to, curblk);
        curblk = package_get_next_datablk(curblk);
    }

    // un//lock(&pkg_locker);
    return 0;
}

int package_write(pkg_t *package, const uint8_t *buf, int len)
{
    // lock(&pkg_locker);
    if (package == NULL || buf == NULL)
    {
        // un//lock(&pkg_locker);
        return -1;
    }
    if (package->pos + len > package->total)
    {
        len = package->total - package->pos; // 限制读取长度
    }

    pkg_dblk_t *blk = package->curblk;
    if (blk == NULL)
    {
        // un//lock(&pkg_locker);
        return -1;
    }

    uint8_t *src = buf;
    int remain_size = len;
    int write_size = 0;
    while (remain_size > 0)
    {
        // 如果当前快够写
        if (remain_size < blk->size - package->inner_offset)
        {
            write_size = remain_size;
        }
        else // 当前块不够写
        {
            // 能写多少写多少
            write_size = blk->size - package->inner_offset;
        }
        uint8_t *dest = &blk->data[blk->offset + package->inner_offset];
        memcpy(dest, src, write_size);
        src += write_size;
        remain_size -= write_size;
        package->inner_offset += write_size;

        if (remain_size > 0 || package->inner_offset + blk->offset >= PKG_DATA_BLK_SIZE)
        {
            blk = package_get_next_datablk(blk);
            if (!blk)
            {
                package->curblk = NULL;
                package->inner_offset = 0;
                break;
            }
            package->curblk = blk;
            package->inner_offset = 0;
        }
    }

    package->pos += len - remain_size; // 总写入位置更新

    // un//lock(&pkg_locker);

    return len - remain_size;
}

int package_read(pkg_t *package, uint8_t *buf, int len)
{
    if (package == NULL || buf == NULL || len <= 0)
        return -1;

    // lock(&pkg_locker);

    if (package->pos + len > package->total)
    {
        len = package->total - package->pos; // 限制读取长度
    }

    pkg_dblk_t *blk = package->curblk;
    if (blk == NULL)
    {
        // un//lock(&pkg_locker);
        return 0; // 数据块为空或偏移非法
    }

    uint8_t *dest = buf;
    int remain_size = len;
    int read_size = 0;

    while (remain_size > 0)
    {
        if (remain_size < blk->size - package->inner_offset)
        {
            read_size = remain_size;
        }
        else
        {
            read_size = blk->size - package->inner_offset;
        }

        uint8_t *src = &blk->data[blk->offset + package->inner_offset];
        memcpy(dest, src, read_size);
        dest += read_size;
        remain_size -= read_size;
        package->inner_offset += read_size;

        if (remain_size > 0 || package->inner_offset + blk->offset >= PKG_DATA_BLK_SIZE)
        {
            blk = package_get_next_datablk(blk);
            if (!blk)
            {
                package->curblk = NULL;
                package->inner_offset = 0;
                break;
            }
            package->curblk = blk;
            package->inner_offset = 0;
        }
    }

    package->pos += len - remain_size; // 总读取位置更新
    // un//lock(&pkg_locker);

    return len - remain_size; // 返回实际读取的字节数
}

int package_write_pos(pkg_t *package, const uint8_t *buf, int len, int position)
{
    int ret;
    // lock(&pkg_locker);

    if (position >= 0)
    {
        ret = package_lseek(package, position);
        if (ret < 0)
        {
            // un//lock(&pkg_locker);
            dbg_error("pkg_lseek wrong\r\n");
            return ret;
        }
    }
    ret = package_write(package, buf, len);
    if (ret < 0)
    {
        // un//lock(&pkg_locker);
        dbg_error("pkg write wrong\r\n");
        return ret;
    }
    // un//lock(&pkg_locker);
    return 0;
}
int package_read_pos(pkg_t *package, uint8_t *buf, int len, int position)
{
    int ret;
    // lock(&pkg_locker);
    if (position >= 0)
    {
        ret = package_lseek(package, position);
        if (ret < 0)
        {
            // un//lock(&pkg_locker);
            dbg_error("pkg_lseek wrong\r\n");
            return ret;
        }
    }
    ret = package_read(package, buf, len);
    if (ret < 0)
    {
        // un//lock(&pkg_locker);
        dbg_error("pkg read wrong\r\n");
        return ret;
    }
    // un//lock(&pkg_locker);
    return 0;
}
int package_lseek(pkg_t *package, int offset)
{
    // lock(&pkg_locker);
    if (package == NULL || offset < 0 || offset > package->total)
    {
        // un//lock(&pkg_locker);
        return -1;
    }

    pkg_dblk_t *blk = package_get_first_datablk(package); // 从链表头开始查找
    package->curblk = blk;
    package->inner_offset = 0;
    int remain_size = offset;
    package->pos = offset;
    while (remain_size >= 0)
    {
        if (remain_size < blk->size)
        {
            package->curblk = blk;
            package->inner_offset = remain_size;
            break;
        }
        remain_size -= blk->size;
        blk = package_get_next_datablk(blk);
        package->inner_offset = 0;
    }

    // un//lock(&pkg_locker);
    return 0;
}

int package_memcpy(pkg_t *dest_pkg, int dest_offset, pkg_t *src_pkg, int src_offset, int len)
{
    if (dest_pkg == NULL || src_pkg == NULL || len <= 0)
    {
        return -1;
    }
    int ret;
    // lock(&pkg_locker);

    package_lseek(dest_pkg, dest_offset);
    package_lseek(src_pkg, src_offset);

    uint8_t *buf = malloc(len);

    ret = package_read(src_pkg, buf, PKG_DATA_BLK_SIZE);
    package_write(dest_pkg, buf, ret);

    // un//lock(&pkg_locker);

    return 0;
}

int package_memset(pkg_t *package, int offset, uint8_t value, int len)
{
    if (package == NULL || offset < 0 || len <= 0 || offset + len > package->total)
    {
        return -1;
    }
    int ret;

    uint8_t *arr = malloc(len);
    memset(arr, 0, len);
    // lock(&pkg_locker);
    ret = package_write_pos(package, arr, len, offset);
    if (ret < 0)
    {
        // un//lock(&pkg_locker);
        free(arr);
        return ret;
    }

    // un//lock(&pkg_locker);
    free(arr);

    return 0;
}

pkg_t *package_create(uint8_t *data_buf, int len)
{
    int ret;
    if (data_buf == NULL)
        return NULL;
    pkg_t *pkg = package_alloc(len);
    if (pkg == NULL)
    {
        return NULL;
    }
    ret = package_lseek(pkg, 0);
    if (ret != 0)
    {
        package_collect(pkg);
        return NULL;
    }
    ret = package_write(pkg, data_buf, len);
    if (ret < 0)
    {
        package_collect(pkg);
        return NULL;
    }
    ret = package_lseek(pkg, 0); // 写完后，让指针默认回到开头处
    if (ret != 0)
    {
        package_collect(pkg);
        return NULL;
    }
    return pkg;
}

int package_add_header(pkg_t *package, uint8_t *head_buf, int head_len)
{
    if (package == NULL || head_buf == NULL)
    {
        return -1;
    }
    int ret;
    ret = package_add_headspace(package, head_len);
    if (ret < 0)
    {
        return ret;
    };
    ret = package_lseek(package, 0);
    if (ret < 0)
    {
        return ret;
    };
    ret = package_write(package, head_buf, head_len);
    if (ret < 0)
    {
        return ret;
    };
    return 0;
}

int package_copy(pkg_t *dest_pkg, pkg_t *src_pkg)
{
    return package_memcpy(dest_pkg, 0, src_pkg, 0, src_pkg->total);
}
void package_print(pkg_t *pkg)
{
    uint8_t *rbuf = malloc(pkg->total);
    package_lseek(pkg, 0);
    package_read(pkg, rbuf, pkg->total);
    for (int i = 0; i < pkg->total; ++i)
    {

        dbg_info("%x ", rbuf[i]);
    }
    dbg_info("\r\n");
    free(rbuf);
}

/*整合数据包头,并返回所需数据position的首地址*/
uint8_t *package_data(pkg_t *pkg, int len, int position)
{
    int x;
    if (!pkg || !pkg->curblk)
    {
        return NULL; // 检查空指针情况
    }
    if (len < 0 || len > PKG_DATA_BLK_SIZE)
    {
        dbg_error("len is wrong\r\n");
        return NULL;
    }
    if (position >= 0)
    {
        x = package_lseek(pkg, position);
        if (x < 0)
        {
            dbg_error("pkg lseek err\r\n");
            return NULL;
        }
    }
    package_integrate_header(pkg, len);
    package_lseek(pkg, pkg->pos); // 数据包头整合后，curk或者pos等会受影响，重新定位
    uint8_t *ret = &pkg->curblk->data[pkg->curblk->offset + pkg->inner_offset];
    return ret;
}