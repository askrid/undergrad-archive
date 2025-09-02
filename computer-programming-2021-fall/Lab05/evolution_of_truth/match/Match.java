package evolution_of_truth.match;

import evolution_of_truth.agent.Agent;

public class Match {
    
    public static int CHEAT = 0;
    public static int COOPERATE = 1;
    public static int UNDEFINED = -1;

    // Sets the value each player gets for all possible cases
    protected static int ruleMatrix[][][] = {
        {
            {0, 0},
            {3, -1}
        },
        {
            {-1, 3},
            {2, 2}
        }
    };

    protected Agent agentA, agentB;
    protected int previousChoiceA, previousChoiceB, ppreviousChoiceA, ppreviousChoiceB;

    public Match(Agent agentA, Agent agentB) {
        this.agentA = agentA;
        this.agentB = agentB;
        previousChoiceA = UNDEFINED;
        previousChoiceB = UNDEFINED;
    }

    public void playGame() {
        int choiceA = agentA.choice(previousChoiceB, ppreviousChoiceB);
        int choiceB = agentB.choice(previousChoiceA, ppreviousChoiceA);

        ppreviousChoiceA = previousChoiceA;
        ppreviousChoiceB = previousChoiceB;
        previousChoiceA = choiceA;
        previousChoiceB = choiceB;

        agentA.setScore(agentA.getScore() + ruleMatrix[choiceA][choiceB][0]);
        agentB.setScore(agentB.getScore() + ruleMatrix[choiceA][choiceB][1]);
    }
}
