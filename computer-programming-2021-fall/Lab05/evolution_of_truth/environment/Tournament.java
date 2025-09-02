package evolution_of_truth.environment;

import evolution_of_truth.match.*;
import evolution_of_truth.agent.*;
import population.*;

public class Tournament {

    Population agentPopulation;

    public Tournament() {
        agentPopulation = new Population();

        for (int i = 0; i < 15; i++) {
            agentPopulation.addIndividual(new Copykitten());
        }
        for (int i = 15; i < 20; i++) {
            agentPopulation.addIndividual(new Devil());
        }
        for (int i = 20; i < 25; i++) {
            agentPopulation.addIndividual(new Copycat());
        }
    }
    
    private Match[] createAllMatches() {
        int n = agentPopulation.size();

        Individual[] agents = agentPopulation.getIndividuals();
        Match[] matches = new Match[n* (n-1) / 2];

        int idx = 0;
        for (int i = 0; i < n; i++) {
            for (int j = i+1; j < n; j++) {
                matches[idx++] = new MistakeMatch((Agent) agents[i], (Agent) agents[j]);
            }
        }

        return matches;
    }

    public void playAllGames(int numRounds) {
        Match[] matches = createAllMatches();

        for (int i = 0; i < numRounds; i++) {
            for (Match match : matches) {
                match.playGame();
            }
        }
    }

    public void describe() {
        Individual[] agents = agentPopulation.getIndividuals();

        for (Individual _agent : agents) {
            Agent agent = (Agent) _agent;
            System.out.print(agent.toString() + " / ");
        }

        System.out.println();
    }

    public void evolvePopulation() {
        agentPopulation.toNextGeneration(5);
    }

    public void resetAgents() {
        Individual[] agents = agentPopulation.getIndividuals();
        
        for (Individual _agent : agents) {
            Agent agent = (Agent) _agent;
            agent.setScore(0);
        }
    }
}
