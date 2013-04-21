kernel() {
	runnable= List<Process> 
    waitingForAllocate = Map<Process, Integer>
    waitingForMovement = Map<Process, Integer> 
	waiting= MultiMap<SyscallTag, Pair<Process, Context>>
	schedule = Map<pid, time_t>
	while true {
		for (item : waiting[sleepTag]) {
	        if (shedule[item.pid] <= currentTime) {
                waiting.remove(item);
                runnable.add(item);    
            }    
        }	
        for (item : waitingForMovement) {
            if ((movedptr = isMoved(waitingForMovement.getOperationId(item))) != NULL) {
                Allocator allocator;
                allocator.inRam(movedptr, false);
                newptr = allocator.allocate(k);
                allocator.addFirst(newptr, k);
                allocator.inRam(newptr, true);
                runnable.append(item);
            }
        }
        curProc = runnable.first();
        context = curProc.exec()
        switch context.tag {
            case sleepTag:
                waiting.put(sleepTag, new Pair(curProc, context));
                break
            case allocTag:
                k = context.argv[0]
                Allocator allocator;
                if (allocator.haveInRAM(k)) {
                    ptr = allocator.allocate(k);
                    allocator.addFirst(ptr, k);
                    allocator.inRam(ptr, true);
                    runnable.append(new Process(..., context.cont(ptr)))
                    break;
                }
                if (allocator.haveInHard(k)) {
                    opId = moveToHardDisk(allocator.last());
                    waitingForMovement.add(process, opId);
                } else {
                    waitingForAllocate.add(curProc, k);
                }
                break

            case freeTag:
                ptr = context.argv[0]
                Allocator allocator;
                runnable.append(new Process(..., context.cont()))
                if (allocator.inRam(ptr)) {
                    allocator.releaseInRam(ptr);
                    allocator.removeFromMap(ptr);
                    for (item : waitingForAllocate) {
                        int k = waitingForAllocate.get(item);
                        if (allocator.haveInRAM(k)) {
                            ptr = allocator.allocate(k);
                            allocator.addFirst(ptr, k);
                            allocator.inRam(ptr, true);
                            runnable.append(item);
                        }

                    }
                } else {
                    allocator.releaseInHard(ptr);
                    allocator.removeFromMap(ptr);

                    for (item : waitingForAllocate) {
                        int k = waitingForAllocate.get(item);
                        if (allocator.haveInHard(k)) {
                            opId = moveToHardDisk(allocator.last());
                            waitingForMovement.add(process, opId);
                        }

                    }
                }
                break;
        }
    } 

}
