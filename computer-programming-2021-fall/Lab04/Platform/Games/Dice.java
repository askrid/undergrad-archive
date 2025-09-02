package Platform.Games;

import java.lang.Math;

public class Dice {
    
    public int playGame() {
        int user = getRandomDice();
        int opp = getRandomDice();

        System.out.println(user + " " + opp);

        return getResult(user, opp);
    }

    private int getRandomDice() {
        int dice = (int) (Math.random() * 100);

        return dice;
    }

    private int getResult(int user, int opp) {
        if (user > opp) {
            return 1;
        } else if (user < opp) {
            return -1;
        } else {
            return 0;
        }
    }
}
