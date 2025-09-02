public class FibonacciNumbers {
    public static void printFibonacciNumbers(int n) {
        // DO NOT change the skeleton code.
        // You can add codes anywhere you want.

        int a = 0, b = 1;
        long sum = 0;

        for (int i = 0; i < n; i++) {
          System.out.printf("%d", a);
          sum += a;

          b += a;
          a = b - a;

          if (i < n-1) {
            System.out.printf(" ");
          }
        }
        System.out.println();

        if (sum < 100000) {
          System.out.printf("sum = %d\n", sum);
        } else {
          String strsum = String.valueOf(sum);
          System.out.printf("last five digits of sum = %s\n", strsum.substring(strsum.length()-5, strsum.length()));
        }
    }
}
