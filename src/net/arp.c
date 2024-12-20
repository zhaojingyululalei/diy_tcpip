#include "arp.h"

static arp_cache_table_t arp_cache_table;

static void arp_cache_table_init(arp_cache_table_t* table)
{
    memset(table,0,sizeof(arp_cache_table_t));
    list_init(&table->entry_list);
    mempool_init(&table->entry_pool,table->entry_buff,ARP_ENTRY_MAX_SIZE,sizeof(arp_entry_t));
}

void arp_init(void)
{
    arp_cache_table_init(&arp_cache_table);
}