import java.io.BufferedReader;
import java.io.FileReader;
import java.util.StringTokenizer;
import java.io.PrintWriter;
import java.lang.Math;

public class Solution1 {

    static int count, n, e, answer;
    static int[][] graph;
    static int[][][] d;
    
    public static void main(String[] args) throws Exception {

        BufferedReader br = new BufferedReader(new FileReader("input1.txt"));
        StringTokenizer stk;
        PrintWriter pw = new PrintWriter("output1.txt");

        stk = new StringTokenizer(br.readLine());
        count = 0;

        while (stk != null) {
            count++;
            n = Integer.parseInt(stk.nextToken());
            e = Integer.parseInt(stk.nextToken());

            graph = new int[n][n];
            for (int i = 0; i < n; i++)
                for (int j = i+1; j < n; j++) {
                    graph[i][j] = Integer.MAX_VALUE;
                    graph[j][i] = Integer.MAX_VALUE;
                }

            stk = new StringTokenizer(br.readLine());
            int start, end, weight;

            while (stk.hasMoreTokens()) {
                start = Integer.parseInt(stk.nextToken()) - 1;
                end = Integer.parseInt(stk.nextToken()) - 1;
                weight = Integer.parseInt(stk.nextToken());
                graph[start][end] = weight;
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
        d = new int[n][n][n+1];

        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                d[i][j][0] = graph[i][j];
            }
        }
        
        int value;

        for (int k = 1; k <= n; k++) {
            for (int i = 0; i < n; i++) {
                for (int j = 0; j < n; j++) {
                    // inf + x = inf
                    if (d[i][k-1][k-1] == Integer.MAX_VALUE || d[k-1][j][k-1] == Integer.MAX_VALUE)
                        value = Integer.MAX_VALUE;
                    else
                        value = d[i][k-1][k-1] + d[k-1][j][k-1];

                    d[i][j][k] = Math.min(d[i][j][k-1], value);
                }
            }
        }

        answer = 0;
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++)
                answer += d[i][j][n];
        }
        answer %= 100000000;
    }
}