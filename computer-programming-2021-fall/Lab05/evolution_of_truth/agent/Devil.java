package evolution_of_truth.agent;

import evolution_of_truth.match.Match;
import population.Individual;

public class Devil extends Agent {

    public Devil() {
        super("Devil");
    }
    
    @Override
    public int choice(int previousOpponentChoice, int ppreviousOpponentChoice) {
        return Match.CHEAT;
    }

    @Override
    public Individual clone() {
        return new Devil();
    }
}
