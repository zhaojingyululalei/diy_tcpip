#include "include/package.h"
#include "include/mempool.h"
#include "thread.h"
static uint8_t pkg_buf[(sizeof(list_node_t) + sizeof(pkg_t)) * PKG_LIMIT];
static uint8_t pkg_databuf[(sizeof(list_node_t) + sizeof(pkg_dblk_t)) * PKG_DATA_BLK_LIMIT];

static lock_t pkg_locker;
static mempool_t pkg_pool;
static mempool_t pkg_datapool;

net_err_t package_pool_init(void)
{
    net_err_t ret;
    printf("package pool init ing ...\n");

    ret = lock_init(&pkg_locker);
    CHECK_NET_ERR(ret);
    ret = mempool_init(&pkg_pool, pkg_buf, PKG_LIMIT, sizeof(pkg_t));
    CHECK_NET_ERR(ret);
    ret = mempool_init(&pkg_datapool, pkg_databuf, PKG_DATA_BLK_LIMIT, sizeof(pkg_dblk_t));
    CHECK_NET_ERR(ret);
    printf("package pool init finsh!...\n");
    return NET_ERR_OK;
}
net_err_t package_pool_destory(void)
{
    net_err_t ret;
    printf("package pool destory...\n");
    ret = lock_destory(&pkg_locker);
    CHECK_NET_ERR(ret);
    mempool_destroy(&pkg_pool);
    mempool_destroy(&pkg_datapool);
    printf("package pool destory finish! ...\n");
    return NET_ERR_OK;
}

static void package_init(pkg_t *pkg, int size)
{
    lock(&pkg_locker);
    pkg->total = size;
    list_init(&pkg->pkgdb_list);
    pkg->node.next = NULL;
    pkg->node.pre = NULL;
    unlock(&pkg_locker);
}
static void package_destory(pkg_t *package)
{
    lock(&pkg_locker);
    package->total = 0;
    package->node.next = NULL;
    package->node.pre = NULL;
    list_destory(&package->pkgdb_list);
    unlock(&pkg_locker);
}
#include <string.h>
static void package_datablk_destory(pkg_dblk_t *blk)
{
    lock(&pkg_locker);
    blk->node.next = NULL;
    blk->node.pre = NULL;
    blk->offset = 0;
    blk->size = 0;
    memset(blk->data, 0, PKG_DATA_BLK_SIZE);
    unlock(&pkg_locker);
}

net_err_t package_collect(pkg_t *package)
{
    net_err_t ret;
    CHECK_NET_NULL(package);
    lock(&pkg_locker);
    // 先回收数据块
    while (list_count(&package->pkgdb_list) > 0)
    {

        list_node_t *dnode = list_remove_first(&package->pkgdb_list);
        pkg_dblk_t *blk = list_node_parent(dnode, pkg_dblk_t, node);
        package_datablk_destory(blk);
        ret = mempool_free_blk(&pkg_datapool, blk);
        if (ret != NET_ERR_OK)
        {
            unlock(&pkg_locker);
            return ret;
        }
    }
    package_destory(package);
    ret = mempool_free_blk(&pkg_pool, package);
    unlock(&pkg_locker);
    return NET_ERR_OK;
}

pkg_t *package_alloc(int size)
{
    net_err_t ret;
    pkg_t *package = mempool_alloc_blk(&pkg_pool, -1);
    if (package == NULL)
    {
        return NULL;
    }
    lock(&pkg_locker);
    package_init(package, size);

    while (size > 0)
    {
        pkg_dblk_t *dblk = mempool_alloc_blk(&pkg_datapool, -1);
        if (dblk == NULL)
        {
            ret = package_collect(package);
            if (ret != NET_ERR_OK)
                dbg_error("not ok here\n");
            unlock(&pkg_locker);
            return NULL;
        }
        dblk->node.next = NULL;
        dblk->node.pre = NULL;
        dblk->size = (size >= PKG_DATA_BLK_SIZE) ? PKG_DATA_BLK_SIZE : size;
        dblk->offset = 0;
        list_insert_last(&package->pkgdb_list, &dblk->node);
        size -= dblk->size;
    }
    unlock(&pkg_locker);
    return package;
}

net_err_t package_expand_front(pkg_t *package, int ex_size)
{
    lock(&pkg_locker);
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
            unlock(&pkg_locker);
            return NET_ERR_MEM;
        }
        new_blk->size = new_size;
        new_blk->offset = PKG_DATA_BLK_SIZE - new_size;
        list_insert_first(&package->pkgdb_list, &new_blk->node);
    }
    unlock(&pkg_locker);
    return NET_ERR_OK;
}

net_err_t package_expand_last(pkg_t *package, int ex_size)
{
    lock(&pkg_locker);
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
            unlock(&pkg_locker);
            return NET_ERR_MEM;
        }
        int new_size = ex_size - last_leave;
        blk->size = new_size;
        blk->offset = 0;
        list_insert_last(&package->pkgdb_list, &blk->node);
    }
    unlock(&pkg_locker);
    return NET_ERR_OK;
}

net_err_t package_expand_front_align(pkg_t *package, int ex_size)
{
    lock(&pkg_locker);
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
        pkg_dblk_t *new_blk = mempool_alloc_blk(&pkg_datapool, -1);
        if (new_blk == NULL)
        {
            package->total -= ex_size;
            unlock(&pkg_locker);
            return NET_ERR_MEM;
        }
        new_blk->offset = PKG_DATA_BLK_SIZE - ex_size;
        new_blk->size = ex_size;
        list_insert_first(&package->pkgdb_list, &new_blk->node);
    }
    unlock(&pkg_locker);
    return NET_ERR_OK;
}

/*show*/
net_err_t package_show_pool_info(void)
{
    printf("package pool info.....................\n");
    printf("package pool free pkg cnt is %d\n", mempool_freeblk_cnt(&pkg_pool));
    printf("pkg data pool free blk cnt is %d\n", mempool_freeblk_cnt(&pkg_datapool));
    printf("\n");
    return NET_ERR_OK;
}
net_err_t package_show_info(pkg_t *package)
{
    if (package == NULL)
    {
        return NET_ERR_SYS;
    }
    int count = 0, i = 1;
    lock(&pkg_locker);
    list_node_t *cur = &package->node;
    while (cur)
    {
        count++;
        cur = cur->next;
    }
    printf("package info ......................\n");
    printf("pkg total size is: %d\n", package->total);
    printf("pkg brother link list count(include itself): %d\n", count);
    list_node_t *fnode = list_first(&package->pkgdb_list);
    pkg_dblk_t *curblk = list_node_parent(fnode, pkg_dblk_t, node);
    while (curblk)
    {
        printf("blk[%d].size=%d, .offset=%d\n",
               i++, curblk->size, curblk->offset);
        curblk = package_get_next_datablk(curblk);
    }
    printf("pkg sum data block cnt is %d\n", i - 1);
    unlock(&pkg_locker);
    printf("\n");
    return NET_ERR_OK;
}
/**
 * 给数据包添加包头
 */
net_err_t package_add_headspace(pkg_t *package, int h_size)
{
    return package_expand_front_align(package, h_size);
}
pkg_dblk_t *package_get_next_datablk(pkg_dblk_t *cur_blk)
{
    lock(&pkg_locker);
    list_node_t *next_node = cur_blk->node.next;
    if (next_node == NULL)
    {
        unlock(&pkg_locker);
        return NULL;
    }
    pkg_dblk_t *next_blk = list_node_parent(next_node, pkg_dblk_t, node);
    unlock(&pkg_locker);
    return next_blk;
}
pkg_dblk_t *package_get_pre_datablk(pkg_dblk_t *cur_blk)
{
    lock(&pkg_locker);
    list_node_t *pre_node = cur_blk->node.pre;
    if (pre_node == NULL)
    {
        unlock(&pkg_locker);
        return NULL;
    }
    pkg_dblk_t *pre_blk = list_node_parent(pre_node, pkg_dblk_t, node);
    unlock(&pkg_locker);
    return pre_blk;
}

net_err_t package_remove_one_blk(pkg_t *package, pkg_dblk_t *delblk)
{
    lock(&pkg_locker);
    if (package == NULL || delblk == NULL)
    {
        unlock(&pkg_locker);
        return NET_ERR_SYS;
    }
    list_t *list = &package->pkgdb_list;
    list_node_t *delnode = &delblk->node;
    list_remove(list, delnode);
    mempool_free_blk(&pkg_datapool, delblk);
    unlock(&pkg_locker);
    return NET_ERR_OK;
}

pkg_dblk_t *package_get_first_datablk(pkg_t *package)
{
    lock(&pkg_locker);
    list_t *list = &package->pkgdb_list;
    list_node_t *fnode = list_first(list);
    if (fnode == NULL)
    {
        unlock(&pkg_locker);
        dbg_error("pkg has no blk\n");
        return NULL;
    }
    else
    {
        unlock(&pkg_locker);
        return list_node_parent(fnode, pkg_dblk_t, node);
    }
    unlock(&pkg_locker);
    return NULL;
}
pkg_dblk_t *package_get_last_datablk(pkg_t *package)
{
    lock(&pkg_locker);
    list_t *list = &package->pkgdb_list;
    list_node_t *lnode = list_last(list);
    if (lnode == NULL)
    {
        unlock(&pkg_locker);
        dbg_error("pkg has no blk\n");
        return NULL;
    }
    else
    {
        unlock(&pkg_locker);
        return list_node_parent(lnode, pkg_dblk_t, node);
    }
    unlock(&pkg_locker);
    return NULL;
}

net_err_t package_shrank_front(pkg_t *package, int sh_size)
{
    net_err_t ret;
    lock(&pkg_locker);
    if (sh_size > package->total)
    {
        unlock(&pkg_locker);
        dbg_error("sh_size > pkg size");
        return NET_ERR_SYS;
    }
    package->total -= sh_size;

    pkg_dblk_t *fblk = package_get_first_datablk(package);
    if (fblk->size > sh_size)
    {
        fblk->size -= sh_size;
        fblk->offset += sh_size;
    }
    else if (fblk->size == sh_size)
    {
        ret = package_remove_one_blk(package, fblk);
        if (ret != NET_ERR_OK)
            dbg_warning("package remove one blk fail...\n");
    }
    else
    {
        int leave_sh_size = sh_size - fblk->size;
        ret = package_remove_one_blk(package, fblk);
        if (ret != NET_ERR_OK)
            dbg_warning("package remove one blk fail...\n");
        fblk = package_get_first_datablk(package);
        fblk->size -= leave_sh_size;
        fblk->offset += leave_sh_size;
    }
    unlock(&pkg_locker);
    return NET_ERR_OK;
}

net_err_t package_shrank_last(pkg_t *package, int sh_size)
{
    net_err_t ret;
    lock(&pkg_locker);
    if (sh_size > package->total)
    {
        unlock(&pkg_locker);
        dbg_error("sh_size > pkg size");
        return NET_ERR_SYS;
    }
    package->total -= sh_size;

    pkg_dblk_t *lblk = package_get_last_datablk(package);
    if (lblk->size > sh_size)
    {
        lblk->size -= sh_size;
    }
    else if (lblk->size == sh_size)
    {
        ret = package_remove_one_blk(package, lblk);
        if (ret != NET_ERR_OK)
            dbg_warning("package remove one blk fail...\n");
    }
    else
    {
        int leave_sh_size = sh_size - lblk->size;
        ret = package_remove_one_blk(package, lblk);
        if (ret != NET_ERR_OK)
            dbg_warning("package remove one blk fail...\n");
        lblk = package_get_last_datablk(package);
        lblk->size -= leave_sh_size;
    }
    unlock(&pkg_locker);
    return NET_ERR_OK;
}

/**
 * 将所有数据包头整合到一个数据块中
 */
net_err_t package_integrate_header(pkg_t *package, int all_hsize)
{
    if (package == NULL || all_hsize <= 0 || all_hsize > package->total) {
        dbg_error("Invalid package or header size!\n");
        return NET_ERR_SYS; // 参数校验
    }

    if (all_hsize > PKG_DATA_BLK_SIZE) {
        dbg_error("Header size greater than data block size!\n");
        return NET_ERR_SYS;
    }

    lock(&pkg_locker);

    list_node_t *fnode = list_first(&package->pkgdb_list);
    if (fnode == NULL) {
        unlock(&pkg_locker);
        dbg_error("Package data block list is empty!\n");
        return NET_ERR_SYS; // 空包检查
    }

    pkg_dblk_t *fblk = list_node_parent(fnode, pkg_dblk_t, node);

    // 如果第一个数据块足够容纳所有头数据，无需整合
    if (fblk->size >= all_hsize) {
        unlock(&pkg_locker);
        return NET_ERR_OK;
    }

    uint8_t *dest = fblk->data;              // 目标位置
    uint8_t *src = fblk->data + fblk->offset; // 当前块的起始位置
    int copied = fblk->size;                 // 已复制的数据大小

    if (copied > 0) {
        memmove(dest, src, copied);          // 利用 memmove 处理重叠区域
    }

    pkg_dblk_t *next = package_get_next_datablk(fblk);
    if (next == NULL) {
        unlock(&pkg_locker);
        dbg_error("Header size may be incorrect, not enough data blocks!\n");
        return NET_ERR_SYS;
    }

    // 从下一个数据块中补充剩余的头数据
    int remaining = all_hsize - copied;      // 需要补充的数据量
    src = next->data + next->offset;

    if (remaining > next->size) {
        remaining = next->size;              // 防止越界
    }

    memcpy(dest + copied, src, remaining);   // 将下一块数据复制到目标位置

    next->offset += remaining;              // 更新下一个块的偏移和大小
    next->size -= remaining;
    fblk->size = all_hsize;                 // 更新第一个块的大小
    fblk->offset = 0;

    // 如果下一个数据块已空，移除它
    if (next->size == 0) {
        package_remove_one_blk(package, next);
    }

    unlock(&pkg_locker);

    return NET_ERR_OK;
}


bool package_integrate_two_continue_blk(pkg_t *package, pkg_dblk_t *blk)
{
    if (package == NULL || blk == NULL) {
        dbg_error("Invalid package or block!\n");
        return false;
    }

    lock(&pkg_locker);

    pkg_dblk_t *next = package_get_next_datablk(blk);
    if (next == NULL) {
        unlock(&pkg_locker);
        return false; 
    }

    int both_size = blk->size + next->size;
    if (both_size > PKG_DATA_BLK_SIZE) {
        unlock(&pkg_locker);
        return false; 
    }

    // 使用 memcpy 替代手动拷贝
    memmove(blk->data, blk->data + blk->offset, blk->size);
    blk->offset = 0; // 重置偏移
    blk->size = both_size; // 更新总大小

    memcpy(blk->data + blk->size, next->data + next->offset, next->size);

    // 移除 next 数据块
    if (package_remove_one_blk(package, next) != NET_ERR_OK) {
        dbg_warning("Failed to remove the next block\n");
    }

    unlock(&pkg_locker);
    return true;
}


net_err_t package_join(pkg_t *from, pkg_t *to)
{
    if (from == NULL || to == NULL) {
        return NET_ERR_SYS;
    }

    lock(&pkg_locker);

    // 合并两包的总大小
    to->total += from->total;

    // 合并链表
    list_t *list_f = &from->pkgdb_list;
    list_t *list_to = &to->pkgdb_list;
    list_join(list_f, list_to);

    // 清理 from 的内容
    if (package_collect(from) != NET_ERR_OK) {
        unlock(&pkg_locker);
        dbg_warning("Failed to clean the source package\n");
        return NET_ERR_SYS;
    }

    // 整合目标包的数据块
    pkg_dblk_t *curblk = package_get_first_datablk(to);
    while (curblk) {
        package_integrate_two_continue_blk(to, curblk);
        curblk = package_get_next_datablk(curblk);
    }

    unlock(&pkg_locker);
    return NET_ERR_OK;
}




net_err_t package_write(pkg_t *package, uint8_t *buf, int len)
{
    lock(&pkg_locker);
    if (package == NULL || buf == NULL || len > package->total) {
        unlock(&pkg_locker);
        return NET_ERR_SYS; 
    }


    pkg_dblk_t *curk = package->curblk;
    if (curk == NULL || package->inner_offset >= curk->size) {
        unlock(&pkg_locker); 
        return NET_ERR_SYS; 
    }

    uint8_t *src = buf;
    int remaining_len = len;
    int write_size = curk->size - package->inner_offset;

    while (remaining_len > 0) {
        if (curk == NULL) {
            unlock(&pkg_locker); 
            return NET_ERR_SYS; 
        }

        uint8_t *dest = &curk->data[curk->offset + package->inner_offset];
        if (write_size > remaining_len) {
            write_size = remaining_len; 
        }

        memcpy(dest, src, write_size);
        src += write_size;
        remaining_len -= write_size;

        if (remaining_len > 0) { 
            curk = package_get_next_datablk(curk);
            package->curblk = curk;
            package->inner_offset = 0;
            write_size = curk ? curk->size : 0;
        } else {
            package->inner_offset += write_size; 
        }
    }

    package->pos += len; // 总写入位置更新
    unlock(&pkg_locker);

    return NET_ERR_OK;
}

net_err_t package_read(pkg_t *package, uint8_t *buf, int len)
{
    lock(&pkg_locker);
    if (package == NULL || buf == NULL || len <= 0) {
        unlock(&pkg_locker);
        return NET_ERR_SYS; 
    }


    if (package->pos + len > package->total) {
        unlock(&pkg_locker);
        return NET_ERR_SYS; // 防止读取超出总长度
    }

    pkg_dblk_t *curk = package->curblk;
    if (curk == NULL || package->inner_offset >= curk->size) {
        unlock(&pkg_locker);
        return NET_ERR_SYS; // 数据块检查
    }

    uint8_t *dest = buf;
    int remaining_len = len;
    int read_size = curk->size - package->inner_offset;

    while (remaining_len > 0) {
        if (curk == NULL) {
            unlock(&pkg_locker);
            return NET_ERR_SYS; // 数据块耗尽
        }

        if (read_size > remaining_len) {
            read_size = remaining_len; // 当前块能读取的大小
        }

        memcpy(dest, &curk->data[curk->offset + package->inner_offset], read_size);
        dest += read_size;
        remaining_len -= read_size;

        if (remaining_len > 0) { // 读取完当前块，切换到下一块
            curk = package_get_next_datablk(curk);
            package->curblk = curk;
            package->inner_offset = 0;
            read_size = curk ? curk->size : 0;
        } else {
            package->inner_offset += read_size; // 更新块内偏移
        }
    }

    package->pos += len; // 总读取位置更新
    unlock(&pkg_locker);

    return NET_ERR_OK;
}

net_err_t package_lseek(pkg_t *package, int offset)
{
    lock(&pkg_locker);
    if (package == NULL || offset < 0 || offset > package->total) {
        unlock(&pkg_locker);
        return NET_ERR_SYS; 
    }


    pkg_dblk_t *curk = package_get_first_datablk(package); // 从链表头开始查找
    int cumulative_offset = 0;

    while (curk) {
        if (cumulative_offset + curk->size > offset) {
            package->curblk = curk;
            package->inner_offset = offset - cumulative_offset;
            package->pos = offset;
            unlock(&pkg_locker);
            return NET_ERR_OK;
        }

        cumulative_offset += curk->size;
        curk = curk->node.next;
    }

    unlock(&pkg_locker);
    return NET_ERR_SYS; // 未找到合适位置
}

net_err_t package_memcpy(pkg_t *dest_pkg, int dest_offset, pkg_t *src_pkg, int src_offset, int len)
{
    if (dest_pkg == NULL || src_pkg == NULL || len <= 0) {
        return NET_ERR_SYS; 
    }

    lock(&pkg_locker);

   
    if (src_offset < 0 || src_offset + len > src_pkg->total ||
        dest_offset < 0 || dest_offset + len > dest_pkg->total) {
        unlock(&pkg_locker);
        return NET_ERR_SYS;
    }

    
    uint8_t *buffer = (uint8_t *)malloc(len);
    if (buffer == NULL) {
        unlock(&pkg_locker);
        return NET_ERR_SYS; 
    }


   
    if (package_lseek(src_pkg, src_offset) != NET_ERR_OK) {
        free(buffer);
        unlock(&pkg_locker);
        return NET_ERR_SYS;
    }

    if (package_read(src_pkg, buffer, len) != NET_ERR_OK) {
        free(buffer);
        unlock(&pkg_locker);
        return NET_ERR_SYS;
    }

    if (package_lseek(dest_pkg, dest_offset) != NET_ERR_OK) {
        free(buffer);
        unlock(&pkg_locker);
        return NET_ERR_SYS;
    }

    if (package_write(dest_pkg, buffer, len) != NET_ERR_OK) {
        free(buffer);
        unlock(&pkg_locker);
        return NET_ERR_SYS;
    }

    free(buffer);
    unlock(&pkg_locker);

    return NET_ERR_OK;
}

net_err_t package_memset(pkg_t *package, int offset, uint8_t value, int len)
{
    if (package == NULL || offset < 0 || len <= 0 || offset + len > package->total) {
        return NET_ERR_SYS; 
    }

    lock(&pkg_locker);

   
    if (package_lseek(package, offset) != NET_ERR_OK) {
        unlock(&pkg_locker);
        return NET_ERR_SYS;
    }

    int remaining_len = len;
    pkg_dblk_t *curblk = package->curblk;
    if (curblk == NULL) {
        unlock(&pkg_locker);
        return NET_ERR_SYS; 
    }

    uint8_t *dest = &curblk->data[curblk->offset + package->inner_offset];
    int write_size = curblk->size - package->inner_offset;

    while (remaining_len > 0) {
        if (curblk == NULL) {
            unlock(&pkg_locker);
            return NET_ERR_SYS; 
        }

        if (write_size > remaining_len) {
            write_size = remaining_len;
        }

        memset(dest, value, write_size);
        remaining_len -= write_size;

        if (remaining_len > 0) { 
            curblk = package_get_next_datablk(curblk);
            package->curblk = curblk;
            package->inner_offset = 0;
            dest = curblk ? &curblk->data[curblk->offset] : NULL;
            write_size = curblk ? curblk->size : 0;
        } else {
            package->inner_offset += write_size;
        }
    }

    package->pos = offset + len; 
    unlock(&pkg_locker);

    return NET_ERR_OK;
}

pkg_t*  package_create(uint8_t* data_buf,int len)
{
    net_err_t ret;
    if(data_buf == NULL) return NULL;
    pkg_t* pkg = package_alloc(len);
    if(pkg == NULL)
    {
        return NULL;
    }
    ret = package_lseek(pkg,0);
    if(ret != NET_ERR_OK)
    {
        package_collect(pkg);
        return NULL;
    }
    ret = package_write(pkg,data_buf,len);
    if(ret != NET_ERR_OK)
    {
        package_collect(pkg);
        return NULL;
    }
    return pkg;
}

net_err_t package_add_header(pkg_t* package,uint8_t* head_buf,int head_len)
{
    if(package==NULL||head_buf==NULL)
    {
        return NET_ERR_SYS;
    }
    net_err_t ret;
    ret = package_add_headspace(package,head_len);
    CHECK_NET_ERR(ret);
    ret = package_lseek(package,0);
    CHECK_NET_ERR(ret);
    ret = package_write(package,head_buf,head_len);
    CHECK_NET_ERR(ret);
    return NET_ERR_OK;
}