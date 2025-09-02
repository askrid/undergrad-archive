package user;

import course.Bidding;

import java.util.HashMap;

public class User {
    
    public String userId;
    public int usedMileage;
    public int courseNumber;
    public HashMap<Integer, Bidding> bids;

    public User(String userId, int usedMileage, int courseNumber, HashMap<Integer, Bidding> bids) {
        this.userId = userId;
        this.usedMileage = usedMileage;
        this.courseNumber = courseNumber;
        this.bids = bids;
    }

    public boolean hasBidFile() {
        if (bids == null)
            return false;
        return true;
    }

    public void bid(int courseId, int mileage) {
        if (bids.containsKey(courseId)) {
            if (mileage == 0) {
                usedMileage -= bids.get(courseId).mileage;
                courseNumber -= 1;
                bids.remove(courseId);
            } else {
                usedMileage += mileage - bids.get(courseId).mileage;
                bids.get(courseId).mileage = mileage;
            }
        } else {
            if (mileage == 0) {
            } else {
                usedMileage += mileage;
                courseNumber += 1;
                bids.put(courseId, new Bidding(courseId, mileage));
            }
        }
    }
}
