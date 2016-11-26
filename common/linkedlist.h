#ifndef LINKEDLIST_H__
#define LINKEDLIST_H__

class LinkedListItem {
    public:
        LinkedListItem(void *item);
        void *item(void) { return m_data; };
        void *next(void) { return m_next; };
        void setNext(LinkedListItem *next);

    protected:
        void *m_data;
        LinkedListItem *m_next;
};

class LinkedList {
    public:
        LinkedList(void);
        void add(void *item);
        void *head(void);
        void *next(void);

    protected:
        void *currentItem(void);

        LinkedListItem *m_curr;
        LinkedListItem *m_head;
        LinkedListItem *m_tail;
};

#endif
// vim:ts=4:sw=4:ai:et:si:sts=4
