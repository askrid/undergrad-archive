import java.lang.Math;

public class Player {

    String userId;
    int health = 50;

    Player(String userId) {
        this.userId = userId;
    }

    public void attack(Player opponent) {
        int atkpt = (int) (5*Math.random() + 1);
        
        opponent.health = (opponent.health-atkpt >= 0) ? opponent.health-atkpt : 0;
    }

    public void heal() {
        int healpt = (int) (3*Math.random() + 1);
        
        health = (health+healpt <= 50) ? health+healpt : 50;
    }  

    public boolean alive() {
        return health > 0;
    }

    public char getTactic() {
        return (Math.random() < 0.7) ? 'a' : 'h';
    }
}
