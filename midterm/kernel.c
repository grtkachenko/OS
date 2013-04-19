kernel() {
	runnable= List<Process> 
    waitingForAllocate = List<Process>
	waiting= MultiMap<SyscallTag, Pair<Process, Context>>
	schedule = Map<pid, time_t>
	while true {
		for (item : waiting[sleepTag]) {
	        if (shedule[item.pid] <= currentTime) {
                waiting.remove(item);
                runnable.add(item);    
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
                    ptr = allocator.moveLastToHard(k);
                    allocator.inRam(ptr, false);
                    curptr = allocator.allocate(k);
                    allocator.inRam(curptr, true);
                    runnable.append(new Process(..., context.cont(ptr)))
                } else {
                    waitingForAllocate.add(curProc);
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
                        if (allocator.haveInRAM(item.k)) {
                            ptr = allocator.allocate(item.k);
                            allocator.addFirst(ptr, item.k);
                            allocator.inRam(ptr, true);
                            runnable.append(item);
                        }

                    }
                } else {
                    allocator.releaseInHard(ptr);
                    allocator.removeFromMap(ptr);

                    for (item : waitingForAllocate) {
                        if (allocator.haveInHard(item.k)) {
                            ptr = allocator.moveLastToHard(item.k);
                            allocator.inRam(ptr, false);
                            ptr = allocator.allocate(item.k);
                            allocator.addFirst(ptr, item.k);
                            allocator.inRam(ptr, true);
                            runnable.append(item);
                        }

                    }
                }
                break;
            case moveTag:
                k = context.argv[0]
                p = context.argv[1]
                Allocator allocator;
                ptr = allocator.moveLastToHard(k);
                allocator.inRam(ptr, false);
                newptr = allocator.allocate(k);
                allocator.addFirst(newptr, k);
                allocator.inRam(newptr, true);
                runnable.append(p);
        }
    } 

}
