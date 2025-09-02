public class MatrixFlip {
    public static void printFlippedMatrix(char[][] matrix) {
        // DO NOT change the skeleton code.
        // You can add codes anywhere you want.

        int m = matrix.length;
        int n = matrix[0].length;
        char[][] res = new char[m][n];

        for (int i = 0; i < m; i++) {
            for (int j = 0; j < n; j++) {
                res[m - 1 - i][n - 1 - j] = matrix[i][j];
            }
        }

        for (char[] chars : res) {
            for (char c : chars) {
                System.out.print(c);
            }
            System.out.println();
        }
    }
}
