public class DecreasingString {
    public static void printLongestDecreasingSubstringLength(String inputString) {
		// DO NOT change the skeleton code.
		// You can add codes anywhere you want.

        char[] input = inputString.toCharArray();
        int max = 1;
        char prev = 'a';

        int n = 1;
        for (char e : input) {
            if (e < prev) {
                n++;
            } else {
                n = 1;
            }
            max = (n > max) ? n : max;
            prev = e;
        }

        System.out.println(max);
    }
}
