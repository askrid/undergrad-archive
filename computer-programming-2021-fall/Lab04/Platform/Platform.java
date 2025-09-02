package Platform;

import java.lang.Math;
import java.util.Scanner;
import Platform.Games.*;

public class Platform {
    private int rounds = 1;
    private boolean has_set = false;
    private Dice dice;
    private RockPaperScissors rps;

    public float run() {
        Scanner sc = new Scanner(System.in);
        int choice = sc.nextInt();
    
        int num = (int) (Math.random() * 6) + 5;
        setRounds(num);

        int wins = playRounds(choice);
        
        return wins / (float) rounds;
    }
    
    public void setRounds(int rounds) {
        if (!has_set) {
            this.rounds = rounds;
            this.has_set = true;
        }
    }

    private int playRounds(int choice) {
        int wins = 0;

        if (choice == 0) {
            dice = new Dice();
            for (int i=0; i<rounds; i++) {
                if (dice.playGame() == 1) {
                    wins++;
                }
            }
        } else if (choice == 1) {
            rps = new RockPaperScissors();
            for (int i=0; i<rounds; i++) {
                if (rps.playGame() == 1) {
                    wins++;
                }
            }
        }

        return wins;
    }
}