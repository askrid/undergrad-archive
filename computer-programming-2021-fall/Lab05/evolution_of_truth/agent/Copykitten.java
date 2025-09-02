package evolution_of_truth.agent;

import evolution_of_truth.match.Match;
import population.Individual;

public class Copykitten extends Agent {

    public Copykitten() {
        super("Copykitten");
    }

    @Override
    public int choice(int previousOpponentChoice, int ppreviousOpponentChoice) {
        if (previousOpponentChoice == Match.CHEAT && ppreviousOpponentChoice == Match.CHEAT) {
            return Match.CHEAT;
        } else {
            return Match.COOPERATE;
        }
    }

    @Override
    public Individual clone() {
        return new Copykitten();
    }
}
