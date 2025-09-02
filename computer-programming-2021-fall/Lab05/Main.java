import evolution_of_truth.environment.Tournament;

public class Main {
    public static void main(String[] args) {
        Tournament t = new Tournament();

        for (int i = 0; i < 20; i++) {
            t.resetAgents();
            t.playAllGames(10);
            t.describe();
            t.evolvePopulation();
        }
    }
}
