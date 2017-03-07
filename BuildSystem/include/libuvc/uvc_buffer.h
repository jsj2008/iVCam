#ifndef _UVC_BUFFER_H
#define _UVC_BUFFER_H
#include <stdint.h>
#include <stddef.h>

struct list_head;
struct uvc_buffer;
struct uvc_buffer_pool;

struct list_head {
  struct list_head *next, *prev;
};

struct uvc_buffer_list {
  struct list_head head;
};

struct uvc_buffer {
  struct list_head head;
  uint8_t *buf;
  int32_t buf_size;
  int32_t buf_capacity;
  int is_still_image;
  struct uvc_buffer_pool *pool;
};

struct uvc_buffer_pool {
  int32_t allocated_size;
  int32_t pool_capacity;
};

void uvc_buffer_pool_init(struct uvc_buffer_pool *pool);
void uvc_buffer_pool_destroy(struct uvc_buffer_pool *pool);
int  uvc_buffer_pool_alloc(struct uvc_buffer_pool *pool, int32_t size, 
    uint8_t **buf);
void uvc_buffer_pool_free(struct uvc_buffer_pool *pool, uint8_t *buf);

void uvc_buffer_list_init(struct uvc_buffer_list *list);
void uvc_buffer_list_add(struct uvc_buffer_list *list, struct uvc_buffer *buffer);
int  uvc_buffer_list_pop(struct uvc_buffer_list *list, struct uvc_buffer **buffer);
void uvc_buffer_list_free(struct uvc_buffer_list *list);
int  uvc_buffer_list_empty(struct uvc_buffer_list *list);

void uvc_buffer_append_data(struct uvc_buffer *buffer, uint8_t *data, int32_t size);
void uvc_buffer_free(struct uvc_buffer *buffer);
struct uvc_buffer *uvc_buffer_new(int32_t init_size, struct uvc_buffer_pool *pool);

/////////////////////////////////////////////////////////////
static inline void INIT_LIST_HEAD(struct list_head *list)
{
    list->next = list;
    list->prev = list;
}

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:    the pointer to the member.
 * @type:   the type of the container struct this is embedded in.
 * @member: the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({          \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
    (type *)( (char *)__mptr - offsetof(type,member) );})

static inline void __list_add(struct list_head *new_item,
                  struct list_head *prev,
                  struct list_head *next)
{
    next->prev = new_item;
    new_item->next = next;
    new_item->prev = prev;
    prev->next = new_item;
}

/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */
static inline int list_empty(const struct list_head *head)
{
    return head->next == head;
}

/**
 * list_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void list_add(struct list_head *new_item, struct list_head *head)
{
    __list_add(new_item, head, head->next);
}

/**
 * list_add_tail - add a new entry
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void list_add_tail(struct list_head *new_item, struct list_head *head)
{
    __list_add(new_item, head->prev, head);
}

static inline void list_pop_front(struct list_head *head, struct list_head **front)
{
    if(list_empty(head))
    {
        *front = NULL;
        return;
    }

    *front = head->next;
    head->next = head->next->next;
    head->next->prev = head;
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_del(struct list_head * prev, struct list_head * next)
{
    next->prev = prev;
    prev->next = next;
}


/**
 * list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty() on entry does not return true after this, the entry is
 * in an undefined state.
 */
static inline void list_del_entry(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
}


/**
 * list_for_each_safe - iterate over a list safe against removal of list entry
 * @pos:    the &struct list_head to use as a loop cursor.
 * @n:      another &struct list_head to use as temporary storage
 * @head:   the head for your list.
 */
#define list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
        pos = n, n = pos->next)


#endif
