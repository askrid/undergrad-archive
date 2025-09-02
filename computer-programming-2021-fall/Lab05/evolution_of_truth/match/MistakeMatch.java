package evolution_of_truth.match;

import java.lang.Math;
import evolution_of_truth.agent.Agent;

public class MistakeMatch extends Match {
    
    public MistakeMatch(Agent agentA, Agent agentB) {
        super(agentA, agentB);
    }

    private int reverseChoice(int choice) {
        if (choice == Match.COOPERATE) {
            return Match.CHEAT;
        } else if (choice == Match.CHEAT) {
            return Match.COOPERATE;
        } else {
            return Match.UNDEFINED;
        }
    }

    @Override
    public void playGame() {
        int choiceA = agentA.choice(previousChoiceB, ppreviousChoiceB);
        int choiceB = agentB.choice(previousChoiceA, ppreviousChoiceA);
        choiceA = (Math.random() > 0.05) ? choiceA : reverseChoice(choiceA);
        choiceB = (Math.random() > 0.05) ? choiceB : reverseChoice(choiceB);

        ppreviousChoiceA = previousChoiceA;
        ppreviousChoiceB = previousChoiceB;
        previousChoiceA = choiceA;
        previousChoiceB = choiceB;

        agentA.setScore(agentA.getScore() + ruleMatrix[choiceA][choiceB][0]);
        agentB.setScore(agentB.getScore() + ruleMatrix[choiceA][choiceB][1]);
    }
}
