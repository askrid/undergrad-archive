import java.util.StringTokenizer;
import java.io.FileReader;
import java.io.BufferedReader;
import java.io.PrintWriter;

class Solution4 {
	static final int max_n = 1000;
	static final int max_H = 10000;
	static final int mask = 1000000;

	static int n, H;
	static int[] h = new int[max_n], d = new int[max_n - 1];
	static int Answer;
	static int[][] mem = new int[max_n][max_H + 1];

	public static void main(String[] args) throws Exception {
		/*
		 * Read the input from input.txt Write your answer to output.txt
		 */
		BufferedReader br = new BufferedReader(new FileReader("input.txt"));
		StringTokenizer stk;
		PrintWriter pw = new PrintWriter("output.txt");

		for (int test_case = 1; test_case <= 10; test_case++) {
			/*
			 * n is the number of blocks, and H is the max tower height read each height of
			 * the block to h[0], h[1], ... , h[n-1] read the heights of the holes to d[0],
			 * d[1], ... , d[n-2]
			 */
			stk = new StringTokenizer(br.readLine());
			n = Integer.parseInt(stk.nextToken());
			H = Integer.parseInt(stk.nextToken());
			stk = new StringTokenizer(br.readLine());
			for (int i = 0; i < n; i++) {
				h[i] = Integer.parseInt(stk.nextToken());
			}
			stk = new StringTokenizer(br.readLine());
			for (int i = 0; i < n - 1; i++) {
				d[i] = Integer.parseInt(stk.nextToken());
			}

			// Top block num <= i, Tower height == j
			for (int i = 2; i < n; i++) {
				for (int j = 1; j <= H; j++) {
					mem[i][j] = 0;
				}
			}

			for (int i = 0; i < n; i++) {
				mem[i][0] = 1;
			}

			int h_0 = h[0], h_1 = h[1], d_0 = d[0];

			for (int j = 1; j <= H; j++) {

				// i = 0;
				mem[0][j] = (h_0 == j) ? 1 : 0;

				// i = 1;
				int res = mem[0][j];
				res += (j - h_1 == 0) ? 1 : 0;
				res += (j - h_1 + d_0 > 0) ? mem[0][j - h_1 + d_0] : 0;

				mem[1][j] = res;
			}

			int h_i, d_im, res, idx;

			for (int i = 2; i < n; i++) {

				h_i = h[i];
				d_im = d[i - 1];

				for (int j = 1; j <= H; j++) {

					res = mem[i - 1][j];

					idx = j - h_i;
					res += (idx >= 0) ? mem[i - 2][idx] : 0;

					idx = j - h_i + d_im;
					res += (idx > 0) ? mem[i - 1][idx] - mem[i - 2][idx] : 0;

					mem[i][j] = res % mask;
				}
			}

			Answer = -1;
			for (int e : mem[n - 1])
				Answer += e;
			Answer %= mask;

			// Print the answer to output.txt
			pw.println("#" + test_case + " " + Answer);
			// To ensure that your answer is printed safely, please flush the string buffer
			// while running the program
			pw.flush();
		}

		br.close();
		pw.close();
	}
}
