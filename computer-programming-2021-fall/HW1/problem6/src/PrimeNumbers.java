import java.util.stream.IntStream;
import java.lang.Math;

public class PrimeNumbers {
    public static void printPrimeNumbers(int m, int n) {
        // DO NOT change the skeleton code.
        // You can add codes anywhere you want.

        int[] arr = IntStream.range(m, n + 1).toArray();
        int max = (int) Math.sqrt(n);

        sieve(arr, 2);

        for (int i = 3; i < max+1; i += 2) {
            sieve(arr, i);
        }

        for (int e : arr) {
            if (e > 0) {
                System.out.printf("%d ", e);
            }
        }
        System.out.print("\n");
    }

    private static void sieve(int[] arr, int n) {
        for (int i = 0; i < arr.length; i++) {
            if (arr[i]%n == 0 && arr[i] != n) {
                arr[i] = 0;
            }
        }
    }
}