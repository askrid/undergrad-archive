public class NumberCounter {
    public static void countNumbers(String str0, String str1, String str2) {
		// DO NOT change the skeleton code.
		// You can add codes anywhere you want.

        int x = Integer.parseInt(str0);
        int y = Integer.parseInt(str1);
        int z = Integer.parseInt(str2);
        int n = x * y * z;
        int[] res = new int[10];
        int length = 0;

        for (int i = 0; i < 10; i++) {
            res[i] = 0;
        }

        while (n > 0) {
            res[n%10]++;
            n /= 10;
            length++;
        }

        for (int i = 0; i < 10; i++) {
            if (res[i] != 0) {
                printNumberCount(i, res[i]);
            }
        }
        System.out.printf("length: %d\n", length);
    }

    private static void printNumberCount(int number, int count) {
        System.out.printf("%d: %d times\n", number, count);
    }
}
