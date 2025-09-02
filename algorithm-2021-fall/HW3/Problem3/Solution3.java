import java.io.BufferedReader;
import java.io.FileReader;
import java.util.StringTokenizer;
import java.io.PrintWriter;
import java.util.*;

public class Solution3 {

    static int count, n, e;
    static int[] answer;
    static long time;
    static ArrayList<LinkedList<Node>> graph;
    static int[] d;
    static Queue<Integer> q;
    static HashSet<Integer> s;
    
    public static void main(String[] args) throws Exception {

        BufferedReader br = new BufferedReader(new FileReader("input3.txt"));
        StringTokenizer stk;
        PrintWriter pw = new PrintWriter("output3.txt");

        stk = new StringTokenizer(br.readLine());
        count = 0;

        while (stk != null) {
            count++;
            n = Integer.parseInt(stk.nextToken());
            e = Integer.parseInt(stk.nextToken());

            graph = new ArrayList<LinkedList<Node>>();

            for (int i = 0; i < n; i++)
                graph.add(new LinkedList<>());

            stk = new StringTokenizer(br.readLine());
            int start, end, weight;

            while (stk.hasMoreTokens()) {
                start = Integer.parseInt(stk.nextToken()) - 1;
                end = Integer.parseInt(stk.nextToken()) - 1;
                weight = Integer.parseInt(stk.nextToken());
                graph.get(start).add(new Node(end, weight));
            }
            
            pw.println("#" + count);
            time = System.currentTimeMillis();
            solve1();
            time = System.currentTimeMillis() - time;
            for (int i = 0; i < n-1; i++) {
                pw.print(answer[i] + " ");
            }
            pw.println(answer[n-1]);
            pw.println(time);

            time = System.currentTimeMillis();
            solve2();
            time = System.currentTimeMillis() - time;
            for (int i = 0; i < n-1; i++) {
                pw.print(answer[i] + " ");
            }
            pw.println(answer[n-1]);;
            pw.println(time);

            try {
                stk = new StringTokenizer(br.readLine());
            } catch (NullPointerException exception) {
                break;
            }
        }

        br.close();
        pw.close();
    }

    private static void solve1() {
        d = new int[n];
        
        for (int i = 1; i < n; i++)
            d[i] = Integer.MAX_VALUE;

        for (int i = 0; i < n-1; i++) {
            for (int u = 0; u < n; u++) {
                for (Node v : graph.get(u)) {
                    d[v.value] = Math.min(d[v.value], d[u] + v.weight);
                }
            }
        }

        answer = new int[n];
        for (int i = 0; i < n; i++) {
            answer[i] = d[i];
        }
    }

    private static void solve2() {
        d = new int[n];
        q = new LinkedList<>();
        s = new HashSet<>();

        for (int i = 1; i < n; i++)
            d[i] = Integer.MAX_VALUE;

        q.add(0);
        s.add(0);

        int u;
        while (!q.isEmpty()) {
            u =  q.poll();
            for (Node v : graph.get(u)) {
                if (d[u] + v.weight < d[v.value]) {
                    d[v.value] = d[u] + v.weight;
                    if (s.add(v.value))
                        q.add(v.value);
                }
            }
        }

        answer = new int[n];
        for (int i = 0; i < n; i++) {
            answer[i] = d[i];
        }
    }
}

class Node {
    int value;
    int weight;

    Node (int value, int weight) {
        this.value = value;
        this.weight = weight;
    }
}