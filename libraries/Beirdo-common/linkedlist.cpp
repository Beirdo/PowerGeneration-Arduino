#include <Arduino.h>
#include "linkedlist.h"

LinkedListItem::LinkedListItem(void *item)
{
    m_data = item;
    m_next = 0;
}

void LinkedListItem::setNext(LinkedListItem *next)
{
    m_next = next;
}

LinkedList::LinkedList(void)
{
    m_curr = 0;
    m_head = 0;
    m_tail = 0;
}

void LinkedList::add(void *item)
{
    LinkedListItem *llItem = new LinkedListItem(item);
    
    if (!m_head) {
        m_head = llItem;
    }

    if (m_tail) {
        m_tail->setNext(llItem);
    }

    m_tail = llItem;
}

void *LinkedList::head(void)
{
    m_curr = m_head;
    return currentItem();
}

void *LinkedList::currentItem(void)
{
    if (m_curr) {
        return m_curr->item();
    } else {
        return 0;
    }
}

void *LinkedList::next(void)
{
    if (!m_curr) {
        return 0;
    }

    m_curr = m_curr->next();
    return currentItem();
}

// vim:ts=4:sw=4:ai:et:si:sts=4
