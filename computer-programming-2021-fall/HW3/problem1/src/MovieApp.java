import java.util.*;

public class MovieApp {

    private HashMap<String, Movie> movies = new HashMap<>();
    private HashMap<String, User> users = new HashMap<>();
    
    public boolean addMovie(String title, String[] tags) {
        if (findMovie(title) != null || tags.length == 0)
            return false;
        
        movies.put(title, new Movie(title, tags));

        return true;
    }

    public boolean addUser(String name) {
        if (findUser(name) != null)
            return false;
        
        users.put(name, new User(name));

        return true;
    }

    public Movie findMovie(String title) {
        return movies.get(title);
    }

    public User findUser(String username) {
        return users.get(username);
    }

    public List<Movie> findMoviesWithTags(String[] tags) {
        List<Movie> movieList = new LinkedList<>();
        List<String> givenTags = Arrays.asList(tags);

        if (tags == null || tags.length == 0)
            return movieList;

        for (Movie movie : movies.values()) {
            HashSet<String> tagSet = movie.getTags();
            if (tagSet.containsAll(givenTags))
                movieList.add(movie);
        }

        Collections.sort(movieList, (a, b) -> b.toString().compareTo(a.toString()));

        return movieList;
    }

    public boolean rateMovie(User user, String title, int rating) {
        if (user == null || title == null || rating < 1 || rating > 5)
            return false;

        if (!users.values().contains(user) || findMovie(title) == null)
            return false;

        HashMap<Movie, Integer> ratings = user.getRatings();
        ratings.put(findMovie(title), rating);

        return true;
    }

    public int getUserRating(User user, String title) {
        if (user == null || title == null)
            return -1;

        if (!users.values().contains(user) || findMovie(title) == null)
            return -1;

        HashMap<Movie, Integer> ratings = user.getRatings();
        Integer rating = ratings.get(findMovie(title));

        return (rating == null) ? 0 : rating;
    }

    public List<Movie> findUserMoviesWithTags(User user, String[] tags) {
        List<Movie> movieList = new LinkedList<>();

        if (user == null || !users.values().contains(user))
            return movieList;

        movieList = findMoviesWithTags(tags);

        HashMap<Movie, Integer> history = user.getHistory();

        for (Movie movie : movieList) {
            if (history.containsKey(movie))
                history.put(movie, history.get(movie) + 1);
            else
                history.put(movie, 1);
        }

        return movieList;
    }

    public List<Movie> recommend(User user) {
        List<Movie> movieList = new LinkedList<>();
        
        if (user == null || !users.values().contains(user))
            return movieList;

        HashMap<Movie, Integer> history = user.getHistory();

        if (history == null)
            return movieList;

        for (Movie movie : history.keySet()) {
            movieList.add(movie);
        }

        Collections.sort(movieList, (a, b) -> a.toString().compareTo(b.toString()));
        Collections.sort(movieList, (a, b) -> getMedianRating(b).compareTo(getMedianRating(a)));
        Collections.sort(movieList, (a, b) -> history.get(b).compareTo(history.get(a)));

        try {
            movieList = new ArrayList<>(movieList.subList(0, 3));
        } catch (IndexOutOfBoundsException e) {
            assert true;
        }
        
        return movieList;
    }

    private Float getMedianRating(Movie movie) {
        List<Integer> ratings = new ArrayList<>();

        for (User user : users.values()) {
            if (user.getRatings().containsKey(movie))
                ratings.add(user.getRatings().get(movie));
        }

        float res;
        int n = ratings.size();
        
        if (n == 0)
            res = 0;
        else if (n % 2 == 0)
            res = ((float) (ratings.get(n / 2) + ratings.get(n / 2 - 1))) / (float) 2;
        else
            res = ratings.get((n - 1) / 2);
        
        return Float.valueOf(res);
    }
}
