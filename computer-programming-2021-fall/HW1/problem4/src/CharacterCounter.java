public class CharacterCounter {
    public static void countCharacter(String str) {
		// DO NOT change the skeleton code.
		// You can add codes anywhere you want.

        final String ALFA = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        char[] input = str.toCharArray();
        int countUpper, countLower;

        for (char ch : ALFA.toCharArray()) {
            countUpper = countChar(ch, input);
            countLower = countChar(Character.toLowerCase(ch), input);

            if (countUpper > 0 && countLower > 0) {
                System.out.printf("%c: %d times, %c: %d times\n", ch, countUpper, Character.toLowerCase(ch), countLower);
            } else if (countUpper > 0) {
                printCount(ch, countUpper);
            } else if (countLower > 0) {
                printCount(Character.toLowerCase(ch), countLower);
            }
        }
    }

    private static void printCount(char character, int count) {
        System.out.printf("%c: %d times\n", character, count);
    }

    private static int countChar(char ch, char[] str) {
        int count = 0;
        for (char e : str) {
            if (e == ch) {
                count++;
            }
        }
        return count;
    }
}
