import java.util.StringTokenizer;
import java.io.FileReader;
import java.io.BufferedReader;
import java.io.PrintWriter;
import java.lang.Math;

class Solution2 {

	static final int max_n = 100000;

	static int n;
	static String s;
	static int Answer;
	static int[][] mem;

	public static void main(String[] args) throws Exception {
		/*
		   Read the input from input.txt
		   Write your answer to output.txt
		 */
		BufferedReader br = new BufferedReader(new FileReader("input.txt"));
		StringTokenizer stk;
		PrintWriter pw = new PrintWriter("output.txt");

		for (int test_case = 1; test_case <= 10; test_case++) {

			stk = new StringTokenizer(br.readLine());
			n = Integer.parseInt(stk.nextToken());
			s = br.readLine();
			mem = new int[n+1][n];

			for (int i = 2; i <= n; i++){
				mem[i][i-2] = -2;
			}

			for (int i = 1; i <= n-1; i++) {
				mem[i][i-1] = 0;
			}

			for (int i = 0; i <= n-1; i++) {
				mem[i][i] = 1;
			}

			for (int r = 1; r <= n-1; r++) {
				for (int i = 0; i <= n-1-r; i++) {
					int j = i + r;
					int k;

					for (k = i; k <= j-1; k++) {
						if (s.charAt(k) == s.charAt(j)) break;
					}
					
					// k == j if not found; mem[j+1][j-1] = -2
					mem[i][j] = Math.max(mem[i][j-1], mem[k+1][j-1] + 2);
				}
			}

			Answer = mem[0][n-1];
			
			// Print the answer to output.txt
			pw.println("#" + test_case + " " + Answer);
			// To ensure that your answer is printed safely, please flush the string buffer while running the program
			pw.flush();
		}

		br.close();
		pw.close();
	}
}

