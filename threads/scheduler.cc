// scheduler.cc
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would
//	end up calling FindNextToRun(), and that would put us in an
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "debug.h"
#include "scheduler.h"
#include "main.h"

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads.
//	Initially, no ready threads.
//----------------------------------------------------------------------

//<TODO>
// Declare sorting rule of SortedList for L1 & L2 ReadyQueue
// Hint: Funtion Type should be "static int"
static int L1cmp(Thread *x, Thread *y);
static int L2cmp(Thread *x, Thread *y);
//<TODO>

Scheduler::Scheduler()
{
    //	schedulerType = type;
    // readyList = new List<Thread *>;
    //<TODO>
    // Initialize L1, L2, L3 ReadyQueue
    L1ReadyQueue = new SortedList<Thread *>(L1cmp);
    L2ReadyQueue = new SortedList<Thread *>(L2cmp);
    L3ReadyQueue = new List<Thread *>();
    //<TODO>
    toBeDestroyed = NULL;
}

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{
    //<TODO>
    // Remove L1, L2, L3 ReadyQueue
    delete L1ReadyQueue;
    delete L2ReadyQueue;
    delete L3ReadyQueue;
    //<TODO>
    // delete readyList;
}

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void Scheduler::ReadyToRun(Thread *thread)
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    // DEBUG(dbgThread, "Putting thread on ready list: " << thread->getName());

    Statistics *stats = kernel->stats;
    //<TODO>
    // According to priority of Thread, put them into corresponding ReadyQueue.
    // After inserting Thread into ReadyQueue, don't forget to reset some values.
    // Hint: L1 ReadyQueue is preemptive SRTN(Shortest Remaining Time Next).
    // When putting a new thread into L1 ReadyQueue, you need to check whether preemption or not.
    thread->setStatus(READY);
    thread->setWaitTime(0);
    int priority = thread->getPriority();
    if (priority >= 0 && priority < 50) //L3:0-49
    {
        L3ReadyQueue->Append(thread);
        DEBUG(dbgMLFQ, "[InsertToQueue] Tick [" << stats->totalTicks << "]: Thread [" << thread->getID() << "] is inserted into queue L3");
    }
    else if (priority >= 50 && priority < 100) //L2:50-99
    {
        L2ReadyQueue->Insert(thread);
        DEBUG(dbgMLFQ, "[InsertToQueue] Tick [" << stats->totalTicks << "]: Thread [" << thread->getID() << "] is inserted into queue L2");
    }
    else //L1:100-149
    {
        L1ReadyQueue->Insert(thread);
        DEBUG(dbgMLFQ, "[InsertToQueue] Tick [" << stats->totalTicks << "]: Thread [" << thread->getID() << "] is inserted into queue L1");
    }
    //<TODO>
    // readyList->Append(thread);
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

Thread *
Scheduler::FindNextToRun()
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    /*if (readyList->IsEmpty()) {
    return NULL;
    } else {
        return readyList->RemoveFront();
    }*/

    //<TODO>
    Statistics *stats = kernel->stats;
    Thread *nextThread = NULL;
    if (!L1ReadyQueue->IsEmpty())
    {
        nextThread = L1ReadyQueue->RemoveFront();
        DEBUG(dbgMLFQ, "[RemoveFromQueue] Tick [" << stats->totalTicks << "]: Thread [" << nextThread->getID() << "] is removed from queue L1");
    }
    else if (!L2ReadyQueue->IsEmpty())
    {
        nextThread = L2ReadyQueue->RemoveFront();
        DEBUG(dbgMLFQ, "[RemoveFromQueue] Tick [" << stats->totalTicks << "]: Thread [" << nextThread->getID() << "] is removed from queue L2");
    }
    else if (!L3ReadyQueue->IsEmpty())
    {
        nextThread = L3ReadyQueue->RemoveFront();
        DEBUG(dbgMLFQ, "[RemoveFromQueue] Tick [" << stats->totalTicks << "]: Thread [" << nextThread->getID() << "] is removed from queue L3");
    }

    if (nextThread && nextThread != kernel->currentThread)
        DEBUG(dbgMLFQ, "[ContextSwitch] Tick [" << stats->totalTicks << "]: Thread [" << nextThread->getID() << "] is now selected for execution, thread [" << kernel->currentThread->getID() << "] is replaced, and it has executed [" << kernel->currentThread->getRunTime() << "] ticks");
    // a.k.a. Find Next (Thread in ReadyQueue) to Run
    return nextThread;
    //<TODO>
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable kernel->currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//	"finishing" is set if the current thread is to be deleted
//		once we're no longer running on its stack
//		(when the next thread starts running)
//----------------------------------------------------------------------

void Scheduler::Run(Thread *nextThread, bool finishing)
{
    Thread *oldThread = kernel->currentThread;

    //	cout << "Current Thread" <<oldThread->getName() << "    Next Thread"<<nextThread->getName()<<endl;

    ASSERT(kernel->interrupt->getLevel() == IntOff);

    if (finishing)
    { // mark that we need to delete current thread
        ASSERT(toBeDestroyed == NULL);
        toBeDestroyed = oldThread;
    }

#ifdef USER_PROGRAM // ignore until running user programs
    if (oldThread->space != NULL)
    { // if this thread is a user program,

        oldThread->SaveUserState(); // save the user's CPU registers
        oldThread->space->SaveState();
    }
#endif

    oldThread->CheckOverflow(); // check if the old thread
                                // had an undetected stack overflow

    kernel->currentThread = nextThread; // switch to the next thread
    nextThread->setStatus(RUNNING);     // nextThread is now running

    // DEBUG(dbgThread, "Switching from: " << oldThread->getName() << " to: " << nextThread->getName());

    // This is a machine-dependent assembly language routine defined
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".

    cout << "Switching from: " << oldThread->getID() << " to: " << nextThread->getID() << endl;
    SWITCH(oldThread, nextThread);

    // we're back, running oldThread

    // interrupts are off when we return from switch!
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    DEBUG(dbgThread, "Now in thread: " << kernel->currentThread->getID());

    CheckToBeDestroyed(); // check if thread we were running
                          // before this one has finished
                          // and needs to be cleaned up

#ifdef USER_PROGRAM
    if (oldThread->space != NULL)
    {                                  // if there is an address space
        oldThread->RestoreUserState(); // to restore, do it.
        oldThread->space->RestoreState();
    }
#endif
}

//----------------------------------------------------------------------
// Scheduler::CheckToBeDestroyed
// 	If the old thread gave up the processor because it was finishing,
// 	we need to delete its carcass.  Note we cannot delete the thread
// 	before now (for example, in Thread::Finish()), because up to this
// 	point, we were still running on the old thread's stack!
//----------------------------------------------------------------------

void Scheduler::CheckToBeDestroyed()
{
    if (toBeDestroyed != NULL)
    {
        DEBUG(dbgThread, "toBeDestroyed->getID(): " << toBeDestroyed->getID());
        delete toBeDestroyed;
        toBeDestroyed = NULL;
    }
}

//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void Scheduler::Print()
{
    cout << "Ready list contents:\n";
    // readyList->Apply(ThreadPrint);
    L1ReadyQueue->Apply(ThreadPrint);
    L2ReadyQueue->Apply(ThreadPrint);
    L3ReadyQueue->Apply(ThreadPrint);
}

// <TODO>

// Function 1. Function definition of sorting rule of L1 ReadyQueue
static int L1cmp(Thread *x, Thread *y)
{
    return x->getRemainingBurstTime() - y->getRemainingBurstTime();
}

// Function 2. Function definition of sorting rule of L2 ReadyQueue
static int L2cmp(Thread *x, Thread *y)
{
    return x->getID() - y->getID();
}
// Function 3. Scheduler::UpdatePriority()
// Hint:
// 1. ListIterator can help.
// 2. Update WaitTime and priority in Aging situations
// 3. After aging, Thread may insert to different ReadyQueue

void Scheduler::UpdatePriority()
{
    Statistics *stats = kernel->stats;
    ListIterator<Thread *> *iterL1 = new ListIterator<Thread *>(L1ReadyQueue);
    for (; !iterL1->IsDone(); iterL1->Next())
    {
        Thread *thread = iterL1->Item();
        thread->setWaitTime(thread->getWaitTime() + TimerTicks);
        if (thread->getWaitTime() > 400)
        {
            thread->setWaitTime(thread->getWaitTime() - 400);
            if (thread->getPriority() < 140) {
                thread->setPriority(thread->getPriority() + 10);
                DEBUG(dbgMLFQ, "[UpdatePriority] Tick [" << stats->totalTicks << "]: Thread [" << thread->getID() << "] changes its priority from [" << thread->getPriority() - 10 << "] to [" << thread->getPriority() << "]");
            } else {
                thread->setPriority(149);
            }
        }
    }
    ListIterator<Thread *> *iterL2 = new ListIterator<Thread *>(L2ReadyQueue);
    for (; !iterL2->IsDone(); iterL2->Next())
    {
        Thread *thread = iterL2->Item();
        thread->setWaitTime(thread->getWaitTime() + TimerTicks);
        if (thread->getWaitTime() > 400)
        {
            thread->setPriority(thread->getPriority() + 10);
            thread->setWaitTime(thread->getWaitTime() - 400);
            DEBUG(dbgMLFQ, "[UpdatePriority] Tick [" << stats->totalTicks << "]: Thread [" << thread->getID() << "] changes its priority from [" << thread->getPriority() - 10 << "] to [" << thread->getPriority() << "]");
            if (thread->getPriority() > 99)
            {
                L2ReadyQueue->Remove(thread);
                DEBUG(dbgMLFQ, "[RemoveFromQueue] Tick [" << stats->totalTicks << "]: Thread [" << thread->getID() << "] is removed from queue L2");
                L1ReadyQueue->Insert(thread);
                DEBUG(dbgMLFQ, "[InsertToQueue] Tick [" << stats->totalTicks << "]: Thread [" << thread->getID() << "] is inserted into queue L1");
            }
        }
    }
    ListIterator<Thread *> *iterL3 = new ListIterator<Thread *>(L3ReadyQueue);
    for (; !iterL3->IsDone(); iterL3->Next())
    {
        Thread *thread = iterL3->Item();
        thread->setWaitTime(thread->getWaitTime() + TimerTicks);
        if (thread->getWaitTime() > 400)
        {
            thread->setPriority(thread->getPriority() + 10);
            thread->setWaitTime(thread->getWaitTime() - 400);
            DEBUG(dbgMLFQ, "[UpdatePriority] Tick [" << stats->totalTicks << "]: Thread [" << thread->getID() << "] changes its priority from [" << thread->getPriority() - 10 << "] to [" << thread->getPriority() << "]");
            if (thread->getPriority() > 49)
            {
                L3ReadyQueue->Remove(thread);
                DEBUG(dbgMLFQ, "[RemoveFromQueue] Tick [" << stats->totalTicks << "]: Thread [" << thread->getID() << "] is removed from queue L3");
                L2ReadyQueue->Insert(thread);
                DEBUG(dbgMLFQ, "[InsertToQueue] Tick [" << stats->totalTicks << "]: Thread [" << thread->getID() << "] is inserted into queue L2");
            }
        }
    }


}

// <TODO>
