public class Main {
    public static void main(String[] args) {
        Player p1 = new Player("Superman");
        Player p2 = new Player("Batman");

        Fight f = new Fight(p1, p2);

        while (!f.isFinished()) {
            f.proceed();
        }

        System.out.println(f.getWinner().userId + " is the winner!");
    }
}
