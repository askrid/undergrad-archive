import java.util.List;
import java.lang.Math;

public class Scheduler<T extends Worker> {
    private static final int waitms = 400;
    private List<T> workers;

    public Scheduler(List<T> workers) {
        this.workers = workers;
    }

    T schedule() {
        T e = workers.get(0);
        restExcept(e);
        return e;
    }

    T schedule(int index) {
        T e = workers.get(0);
        if (index < workers.size()) {
            e = workers.get(index);
        }
        restExcept(e);
        return e;
    }

    T scheduleRandom() {
        int n = (int) Math.random() * workers.size();
        T e = workers.get(n);
        restExcept(e);
        return e;
    }

    T scheduleFair() {
        T e = workers.get(0);
        if (e != null) {
            for (T w : workers) {
                if (w.rest > e.rest)
                    e = w;
            }
        }
        restExcept(e);
        return e;

    }

    static void delay() {
        try {
            Thread.sleep(waitms);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void restExcept(T e) {
        if (e != null) {
            for (T w : workers)
                w.rest++;
            e.rest = 0;
        }
    }
}
