import java.util.StringTokenizer;
import java.io.FileReader;
import java.io.BufferedReader;
import java.io.PrintWriter;

class Solution1 {

	static int n; // string length
	static String s; // string sequence
	static int[] sn; // parsed string sequence
	static Pair[][] mem; // 2D array for subproblems; half of the space unused
	static final int[][] operationTable = { { 1, 1, 0 }, { 2, 1, 0 }, { 0, 2, 2 } };

	public static void main(String[] args) throws Exception {

		BufferedReader br = new BufferedReader(new FileReader("input.txt"));
		StringTokenizer stk;
		PrintWriter pw = new PrintWriter("output.txt");

		for (int test_case = 1; test_case <= 10; test_case++) {

			stk = new StringTokenizer(br.readLine());
			n = Integer.parseInt(stk.nextToken());
			s = br.readLine();
			sn = parseInput(s, n);

			// mem[i][j] = c(i, j) = (# of a, # of b, # of c)
			mem = new Pair[n + 1][n];

			// Singleton case: c(1, 0) ~ c(n, n-1)
			for (int i = 1; i <= n; i++) {
				mem[i][i - 1] = new Pair(sn[i - 1]);
			}

			// Problem size = r + 1
			// c(i, j) = sum (k = i to j) operate(c(i, k-1), c(k+1, j))
			for (int r = 0; r <= n - 2; r++) {
				for (int i = 1; i <= n - 1 - r; i++) {
					Pair res = new Pair();

					int j = i + r;
					for (int k = i; k <= j; k++) {
						res.add(operate(mem[i][k - 1], mem[k + 1][j]));
					}

					mem[i][j] = res;
				}
			}

			// Get an answer array
			Pair ans = mem[1][n - 1];

			// Print the answer to output.txt
			pw.println("#" + test_case + " " + ans.get(0) + " " + ans.get(1) + " " + ans.get(2));
			// To ensure that your answer is printed safely, please flush the string buffer
			// while running the program
			pw.flush();
		}

		br.close();
		pw.close();
	}

	private static int[] parseInput(String s, int n) {
		int[] res = new int[n];
		int idx = 0;

		for (char c : s.toCharArray()) {
			res[idx++] = c - 'a';
		}

		return res;
	}

	private static Pair operate(Pair p1, Pair p2) {
		Pair res = new Pair();

		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				res.add(operationTable[i][j], p1.get(i) * p2.get(j));
			}
		}

		return res;
	}
}

class Pair {

	private long[] arr;

	Pair() {
		arr = new long[3];
		arr[0] = 0;
		arr[1] = 0;
		arr[2] = 0;
	}

	Pair(int x) {
		arr = new long[3];
		arr[0] = 0;
		arr[1] = 0;
		arr[2] = 0;
		arr[x] = 1;
	}

	long get(int idx) {
		return arr[idx];
	}

	void add(int idx, long n) {
		arr[idx] += n;
	}

	void add(Pair other) {
		arr[0] += other.get(0);
		arr[1] += other.get(1);
		arr[2] += other.get(2);
	}
}
