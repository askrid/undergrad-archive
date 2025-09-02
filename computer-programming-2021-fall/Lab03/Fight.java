public class Fight {

    int timeLimit = 100;
    int currRound = 0;

    Player p1;
    Player p2;

    Fight(Player p1, Player p2) {
        this.p1 = p1;
        this.p2 = p2;
    }

    public void proceed() {
        System.out.println("Round " + currRound);

        if (p1.getTactic() == 'a') {
            p1.attack(p2);
        } else if (p1.getTactic() == 'h') {
            p1.heal();
        }

        if (p2.getTactic() == 'a') {
            p2.attack(p1);
        } else if (p2.getTactic() == 'h') {
            p2.heal();
        }

        currRound++;
        System.out.printf("%s health: %d\n", p1.userId, p1.health);
        System.out.printf("%s health: %d\n", p2.userId, p2.health);
    }

    public boolean isFinished() {
        return !(currRound < timeLimit && p1.alive() && p2.alive());
    }

    public Player getWinner() {
        return (p1.health > p2.health) ? p1 : p2;
    }
}