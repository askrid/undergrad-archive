import java.util.*;

public class FrontEnd {
    private UserInterface ui;
    private BackEnd backend;
    private User user;

    public FrontEnd(UserInterface ui, BackEnd backend) {
        this.ui = ui;
        this.backend = backend;
    }
    
    public boolean auth(String authInfo){
        String id = authInfo.split("\n")[0];
        String password = authInfo.split("\n")[1];
        String realPw = backend.getPassword(id);

        if (realPw == null || !password.toLowerCase().equals(realPw.toLowerCase()))
            return false;

        this.user = new User(id, realPw);

        return true;
    }

    public void post(Pair<String, String> titleContentPair) {
        String title = titleContentPair.key;
        String content = titleContentPair.value;

        Post post = new Post(title, content);

        backend.writePost(user, post);
    }
    
    public void recommend(int N){
        List<String> friends = backend.getAllFriends(user);
        List<Post> posts = new LinkedList<>();

        for (String userId : friends) {
            posts.addAll(backend.getAllPosts(userId));
        }

        Collections.sort(posts, (a, b) -> b.getDate().compareTo(a.getDate()));

        try {
            posts = posts.subList(0, N);
        } catch (IndexOutOfBoundsException e) {}

        for (Post post : posts) {
            ui.println(post.toString());
        }
    }

    public void search(String command) {
        String[] args = command.split("\\s+", 2);
        List<String> kwArgs = Arrays.asList(args[1].split("\\s+"));

        HashSet<String> keywords = new HashSet<>(kwArgs);
        List<Post> posts = backend.getAllPosts();
        HashMap<Post, Integer> postKeywords = new HashMap<>();

        for (Post post : posts) {
            postKeywords.put(post, post.countKeywords(keywords));
        }

        Collections.sort(posts, (a, b) -> b.countContent().compareTo(a.countContent()));
        Collections.sort(posts, (a, b) -> postKeywords.get(b).compareTo(postKeywords.get(a)));

        try {
            posts = posts.subList(0, 10);
        } catch (IndexOutOfBoundsException e) {}

        for (Post post : posts) {
            if (postKeywords.get(post) >= 1)
                ui.println(post.getSummary());
        }
    }
    
    User getUser(){
        return user;
    }
}
