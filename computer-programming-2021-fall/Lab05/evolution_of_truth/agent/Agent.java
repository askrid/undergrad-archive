package evolution_of_truth.agent;

import population.Individual;

abstract public class Agent extends Individual {
    
    private String name;
    private int score;
    
    public Agent(String name) {
        this.name = name;
        score = 0;
    }

    public int getScore() {
        return score;
    }

    public void setScore(int newScore) {
        score = newScore;
    }

    abstract public int choice(int previousOpponentChoice, int ppreviousOpponentChoice);

    @Override
    public int sortKey() {
        return score;
    }

    @Override
    public String toString() {
        return name + ": " + getScore();
    }
}