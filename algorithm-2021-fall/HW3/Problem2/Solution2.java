import java.io.BufferedReader;
import java.io.FileReader;
import java.util.StringTokenizer;
import java.io.PrintWriter;
import java.util.HashSet;
import java.util.PriorityQueue;

public class Solution2 {

    static int count, n, e, answer;
    static int[][] graph;
    static int[] distance;
    static HashSet<Integer> S;
    static PriorityQueue<Pair<Integer, Integer>> maxHeap;
    
    public static void main(String[] args) throws Exception {

        BufferedReader br = new BufferedReader(new FileReader("input2.txt"));
        StringTokenizer stk;
        PrintWriter pw = new PrintWriter("output2.txt");

        stk = new StringTokenizer(br.readLine());
        count = 0;

        while (stk != null) {
            count++;
            n = Integer.parseInt(stk.nextToken());
            e = Integer.parseInt(stk.nextToken());

            graph = new int[n][n];

            stk = new StringTokenizer(br.readLine());
            int start, end, weight;

            while (stk.hasMoreTokens()) {
                start = Integer.parseInt(stk.nextToken()) - 1;
                end = Integer.parseInt(stk.nextToken()) - 1;
                weight = Integer.parseInt(stk.nextToken());
                graph[start][end] = weight;
                graph[end][start] = weight;
            }
            
            solve();
            pw.println("#" + count + " " + answer);

            try {
                stk = new StringTokenizer(br.readLine());
            } catch (NullPointerException exception) {
                break;
            }
        }

        br.close();
        pw.close();
    }

    private static void solve() {
        distance = new int[n];
        S = new HashSet<>();
        maxHeap = new PriorityQueue<>((x, y) -> y.value.compareTo(x.value));

        for (int i = 0; i < n; i++) {
            S.add(i);
        }

        // r: starting vertex
        int r = 0;
        S.remove(r);

        for (int i = 0; i < n; i++) {
            maxHeap.add(new Pair<Integer, Integer>(i, graph[r][i]));
            distance[i] = graph[r][i];
        }

        while (!S.isEmpty()) {
            int x = maxHeap.poll().key;
            S.remove(x);

            for (int i = 0; i < n; i++) {
                if (S.contains(i) && graph[x][i] > distance[i]) {
                    maxHeap.add(new Pair<Integer, Integer>(i, graph[x][i]));
                    distance[i] = graph[x][i];
                }
            }
        }

        answer = 0;
        for (int i = 0; i < n; i++)
            answer += distance[i];
    }
}

class Pair<E, V> {
    E key;
    V value;

    Pair(E key, V value) {
        this.key = key;
        this.value = value;
    }
}