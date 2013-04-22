kernel() {
	runnable= List<Process> 
    waitingForAllocate = Map<Process, Integer>
    waitingForMovement = Map<Process, Integer> 
	while true {
        Hard hard;
        curProc = runnable.first();
        context = curProc.exec();
        switch context.tag {
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
                        if (allocator.haveInHard(k) && !waitingForMovement.contains(item)) {
                            opId = moveToHardDisk(allocator.last());
                            waitingForMovement.add(process, opId);
                        }

                    }
                }
                break;
            case moveTag:
                runnable.add(curProc);
                Hard hard;
                opid = hard.getLastOpId();
                movedptr = hard.getPtr();
                Allocator allocator;
                allocator.inRam(movedptr, false);
                Proccess item = waitingForMovement.get(opid);
                int k = waitingForMovement.get(item);
                newptr = allocator.allocate(k);
                allocator.addFirst(newptr, k);
                allocator.inRam(newptr, true);
                waitingForMovement.remove(item);
                runnable.append(item)
        }

    } 

}
