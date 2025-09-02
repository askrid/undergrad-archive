import java.util.StringTokenizer;
import java.io.FileReader;
import java.io.BufferedReader;
import java.io.PrintWriter;
import java.lang.Math;

class Solution3 {
	static final int max_n = 1000000;

	static int n;
	static int[][] A = new int[3][max_n];
	static int Answer;
	static int[][] mem;
	static final int[][] compatibleTable = { { 3, 4 }, { 2, 5 }, { 1, 5 }, { 0, 4 }, { 0, 3 }, { 1, 2 } };
	static final int[][] weightTable = { { 1, 0, -1 }, { 1, -1, 0 }, { 0, 1, -1 }, { 0, -1, 1 }, { -1, 1, 0 },
			{ -1, 0, 1 } };

	public static void main(String[] args) throws Exception {
		/*
		 * Read the input from input.txt Write your answer to output.txt
		 */
		BufferedReader br = new BufferedReader(new FileReader("input.txt"));
		StringTokenizer stk;
		PrintWriter pw = new PrintWriter("output.txt");

		for (int test_case = 1; test_case <= 10; test_case++) {

			stk = new StringTokenizer(br.readLine());
			n = Integer.parseInt(stk.nextToken());
			for (int i = 0; i < 3; i++) {
				stk = new StringTokenizer(br.readLine());
				for (int j = 0; j < n; j++) {
					A[i][j] = Integer.parseInt(stk.nextToken());
				}
			}
			mem = new int[n][6];

			int[] arr = { A[0][0], A[1][0], A[2][0] };
			for (int type = 0; type < 6; type++) {
				mem[0][type] = calculate(type, arr);
			}

			for (int i = 1; i <= n - 1; i++) {
				arr[0] = A[0][i];
				arr[1] = A[1][i];
				arr[2] = A[2][i];

				for (int type = 0; type < 6; type++) {
					mem[i][type] = calculate(type, arr) + Math.max(mem[i-1][compatibleTable[type][0]], mem[i-1][compatibleTable[type][1]]);
				}
			}

			Answer = maxArr(mem[n-1]);

			// Print the answer to output.txt
			pw.println("#" + test_case + " " + Answer);
			// To ensure that your answer is printed safely, please flush the string buffer
			// while running the program
			pw.flush();
		}

		br.close();
		pw.close();
	}

	private static int calculate(int type, int[] arr) {
		int res = 0;
		int[] weight = weightTable[type];

		for (int i = 0; i < 3; i++) {
			res += weight[i] * arr[i];
		}

		return res;
	}

	private static int maxArr(int[] arr) {
		int res = arr[0];

		for (int e : arr) {
			res = (e > res) ? e : res;
		}

		return res;
	}
}