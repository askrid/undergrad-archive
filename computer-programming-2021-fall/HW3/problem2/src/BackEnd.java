import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.time.LocalDateTime;
import java.util.*;

public class BackEnd extends ServerResourceAccessible {

    private int postId;

    BackEnd() {
        postId = getMaxId() + 1;
    }

    String getPassword(String id) {
        String password;

        try {
            File file = new File(getServerStorageDir() + id + "/password.txt");
            Scanner scanner = new Scanner(file);
            password = scanner.next();
            scanner.close();
        } catch (FileNotFoundException e) {
            return null;
        }

        return password;
    }

    void writePost(User user, Post post) {
        File[] files = new File(getServerStorageDir() + user.id + "/post/").listFiles();

        if (files.length == 0) {
            post.setId(0);
        } else {
            post.setId(postId++);
        }

        String text = post.getDate() + "\n" + post.getTitle() + "\n\n" + post.getContent();

        try {
            File file = new File(getServerStorageDir() + user.id + "/post/" + post.getId() + ".txt");
            BufferedWriter writer = new BufferedWriter(new FileWriter(file));

            writer.write(text.trim());
            writer.close();
        } catch (IOException e) {
        }
    }

    List<String> getAllFriends(User user) {
        File file = new File(getServerStorageDir() + user.id + "/friend.txt");
        List<String> friends = new LinkedList<>();

        try {
            BufferedReader reader = new BufferedReader(new FileReader(file));

            String line = reader.readLine();

            while (line != null) {
                friends.add(line);
                line = reader.readLine();
            }

            reader.close();
        } catch (IOException e) {
            return new LinkedList<>();
        }

        return friends;
    }

    List<Post> getAllPosts(String id) {
        File[] files = new File(getServerStorageDir() + id + "/post").listFiles();
        List<Post> posts = new LinkedList<>();

        for (File file : files) {
            Post post = parsePost(file);
            if (post != null)
                posts.add(post);
        }

        return posts;
    }

    List<Post> getAllPosts() {
        List<String> users = getAllUsers();
        List<Post> posts = new LinkedList<>();

        for (String userId : users) {
            posts.addAll(getAllPosts(userId));
        }

        return posts;
    }

    List<String> getAllUsers() {
        File[] files = new File(getServerStorageDir()).listFiles();
        List<String> users = new LinkedList<>();

        for (File file : files) {
            users.add(file.getName());
        }

        return users;
    }

    int getMaxId() {
        List<String> users = getAllUsers();

        int maxId = 0;

        for (String userId : users) {
            int id = getUserMaxId(userId);
            maxId = (id > maxId) ? id : maxId;
        }

        return maxId;
    }

    int getUserMaxId(String userId) {
        File[] files = new File(getServerStorageDir() + userId + "/post").listFiles();

        int maxId = 0;

        for (File file : files) {
            if (file.isFile()) {
                int id = getFileId(file);
                maxId = (id > maxId) ? id : maxId;
            }
        }

        return maxId;
    }

    Post parsePost(File file) {
        Post post;

        try {
            BufferedReader reader = new BufferedReader(new FileReader(file));

            int id = getFileId(file);

            String dateString = reader.readLine();
            LocalDateTime dateTime = Post.parseDateTimeString(dateString, Post.getFormatter());

            String title = reader.readLine();

            reader.readLine();
            String content = "";
            String line = reader.readLine();

            while (line != null) {
                content = content.concat(line + "\n");
                line = reader.readLine();
            }

            post = new Post(id, dateTime, title, content);

            reader.close();
        } catch (IOException e) {
            return null;
        }

        return post;
    }

    int getFileId(File file) {
        String name = file.getName();
        int id = Integer.parseInt(name.replaceAll("\\D+", ""));

        return id;
    }
}
