//
//  sjf_dataStructures.h
//  sjf_verb
//
//  Created by Simon Fay on 28/04/2024.
//

#ifndef sjf_dataStructures_h
#define sjf_dataStructures_h

namespace sjf::dataStructures
{
    /** A simple linked list. Can be used as A FIFO queue, also has ability to be used for thread safe purposes */
    template< typename T >
    class linkedList
    {
    private:
        /** Stores the basic node of the list, each node only contains a pointer to the object it refers to anda pointer to the next node in the list */
        struct node
        {
            T* thisObject = nullptr;
            node* theNextObject  = nullptr;
        };
        
    public:
        linkedList(){}
        ~linkedList()
        {
            while( head != nullptr )
                popNode(); // clear list
        }
        
        /** Add a new node to the end of the list
         Arguments:
                --> a pointer to the object to be added to the list
                --> (optional) a bool to state whether or not repeats are allowed
         */
        void addNode( T* objectToAddToList, bool canHaveRepeats = false )
        {
            auto *n = new node();
            n->thisObject = objectToAddToList;
            
            if( head == nullptr )
            {
                head = n;
                tail = n;
                return;
            }
            else if( !canHaveRepeats )
            {
                auto prev = head;
                while ( prev != nullptr )
                {
                    if( n == prev )
                        return;
                    prev = prev->theNextObject;
                }
            }
            tail->theNextObject = n;
            tail = n;
        }
        
        /**
         Pop a node from the beginning of the list
          Returns the pointer th=o the object removed from the list
         */
        T* popNode()
        {
            if ( head != nullptr )
            {
                auto second = head->theNextObject;
                auto toReturn = head->thisObject;
                delete head;
                head = second;
                tail = head == nullptr ? head : tail; // clear tail
                return toReturn;
            }
            return nullptr;
        }
        
        /**
         Provided for easy thread safe abilities
         */
        std::atomic_flag isBusy = ATOMIC_FLAG_INIT;
        
    private:
        /** pointer to the first node of the list */
        node* head = nullptr;
        /** pointer to the last node of the list */
        node* tail = nullptr;
    };
}
#endif /* sjf_dataStructures_h */
