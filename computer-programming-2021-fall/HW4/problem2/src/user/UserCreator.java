package user;

import course.Bidding;

import java.io.*;
import java.util.HashMap;

public class UserCreator {

    public String userId;
    public int usedMileage;
    public int courseNumber;
    public HashMap<Integer, Bidding> bids;

    public UserCreator(String userId, File bidFile) {
        parseArgs(userId, bidFile);
    }

    private void parseArgs(String userId, File bidFile) {
        try (BufferedReader br = new BufferedReader(new FileReader(bidFile))) {
            this.userId = userId;
            this.usedMileage = 0;
            this.courseNumber = 0;
            this.bids = new HashMap<>();

            String line = br.readLine();
            while (line != null) {
                String[] data = line.split("\\|");
                int courseId = Integer.parseInt(data[0]);
                int mileage = Integer.parseInt(data[1]);
                Bidding bid = new Bidding(courseId, mileage);

                this.usedMileage += mileage;
                this.courseNumber += 1;
                this.bids.put(courseId, bid);

                line = br.readLine();
            }

        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public User create() {
        return new User(userId, usedMileage, courseNumber, bids);
    }
}
