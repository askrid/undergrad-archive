package Platform.Games;

import java.util.Scanner;

public class RockPaperScissors {
    
    private enum Gesture {
        ROCK("rock"),
        PAPER("paper"),
        SCISSOR("scissor");

        final private String name;

        private Gesture(String name) {
            this.name = name;
        }

        public String getName() {
            return name;
        }
    }

    public int playGame() {
        Scanner sc = new Scanner(System.in);
        String input = sc.next();

        Gesture user = getUserGesture(input);
        Gesture opp = getRandomGesture();

        if (user != null) {
            System.out.println(user.getName() + ' ' + opp.getName());
        } else {
            System.out.println(input + ' ' + opp.getName());
            return -1;
        }

        int diff = user.compareTo(opp);
        return getResult(diff);
    }

    private Gesture getUserGesture(String input) {
        for (Gesture gesture : Gesture.values()) {
            if (gesture.getName().equals(input)) {
                return gesture;
            }
        }
        return null;
    }

    private Gesture getRandomGesture() {
        int n = (int) (Math.random() * 3);

        return Gesture.values()[n];
    }

    private int getResult(int diff) {
        if (diff == 1 || diff == -2) {
            return 1;
        } else if (diff == -1 || diff == 2) {
            return -1;
        } else {
            return 0;
        }
    }

}

