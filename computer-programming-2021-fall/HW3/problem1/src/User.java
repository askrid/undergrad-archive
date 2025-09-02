import java.util.HashMap;

public class User {

    private String username;
    private HashMap<Movie, Integer> ratings;
    private HashMap<Movie, Integer> history;

    public User(String username) {
        this.username = username;
        this.ratings = new HashMap<>();
        this.history = new HashMap<>();
    }

    public HashMap<Movie, Integer> getRatings() {
        return ratings;
    }

    public HashMap<Movie, Integer> getHistory() {
        return history;
    }

    @Override
    public String toString() {
        return username;
    }
}
