public class DrawingFigure {
    public static void drawFigure(int n) {

        // DO NOT change the skeleton code.
        // You can add codes anywhere you want.

        for (int i = 0; i < 2*n+1; i++) {
            for (int j = 0; j < 2*n+1; j++) {
                if ((i <= n) && (j >= n-i && j <= n+i)) {
                    System.out.printf("*");
                } else if ((i > n) && j >= i-n && j <= 3*n-i) {
                    System.out.printf("*");
                } else {
                    System.out.printf(" ");
                }
                if (j < 2*n) {
                    System.out.printf(" ");
                }
            }
            System.out.printf("\n");
        }
    }
}