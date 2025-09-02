import java.util.Queue;
import java.lang.Math;

public abstract class Worker {
    Queue<Work> workQueue;
    static private int cnt = 0;
    public int id;
    public int rest;

    public abstract void run();

    Worker(Queue<Work> workQueue) {
        this.workQueue = workQueue;
        this.id = cnt++;
        this.rest = 0;
    }

    void report(String msg){
        System.out.printf("worker%d ", id);
        System.out.println(msg);
    }

}

class Producer extends Worker {
    Producer(Queue<Work> workQueue) {
        super(workQueue);
    }

    @Override
    public void run() {
        if (workQueue.size() < 20) {
            Work e = new Work();
            workQueue.offer(e);
            super.report("produced work" + e.id);
        } else {
        super.report("failed to produce work");
        }
    }
}

class Consumer extends Worker {
    Consumer(Queue<Work> workQueue) {
        super(workQueue);
    }

    @Override
    public void run() {
        if (!workQueue.isEmpty() && Math.random() < 0.5) {
            Work e = workQueue.poll();
            super.report("consumed work" + e.id);
        } else {
            super.report("failed to consume work");
        }
    }
}
