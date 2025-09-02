import java.util.HashSet;

public class Movie {

    private String title;
    private HashSet<String> tags;

    public Movie(String title) {
        this.title = title;
    }

    public Movie(String title, String[] tags) {
        HashSet<String> tagSet = new HashSet<>();

        for (String tag : tags) {
            tagSet.add(tag);
        }

        this.title = title;
        this.tags = tagSet;
        
    }

    public HashSet<String> getTags() {
        return tags;
    }

    @Override
    public String toString() {
        return title;
    }
}
