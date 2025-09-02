package utils;

import course.*;
import server.*;
import user.*;

import java.io.*;
import java.util.*;

public class FileIO {

    static public HashMap<Integer, Course> fetchCourses(File coursesDir) {

        HashMap<Integer, Course> courses = new HashMap<>();

        for (File collegeDir : coursesDir.listFiles()) {
            if (collegeDir.isDirectory()) {
                String collegeName = collegeDir.getName();

                for (File courseFile : collegeDir.listFiles()) {
                    if (courseFile.isFile()) {
                        int courseId = Integer.parseInt(courseFile.getName().replace(".txt", ""));

                        CourseCreator cc = new CourseCreator(collegeName, courseId, courseFile);
                        courses.put(courseId, cc.create());
                    } else
                        continue;
                }
            } else
                continue;
        }

        return courses;
    }

    static public HashMap<String, User> fetchUsers(File usersDir) {

        HashMap<String, User> users = new HashMap<>();

        for (File userDir : usersDir.listFiles()) {
            if (userDir.isDirectory()) {
                String userId = userDir.getName();

                File bidFile = new File(userDir.getPath() + "/bid.txt");

                if (bidFile.isFile()) {
                    UserCreator uc = new UserCreator(userId, bidFile);
                    users.put(userId, uc.create());
                } else {
                    users.put(userId, new User(userId, 0, 0, null));
                }
            } else
                continue;
        }

        return users;
    }

    static public void applyBids(User user) {
        File bidFile = new File("data/Users/" + user.userId + "/" + "bid.txt");

        try (BufferedWriter bw = new BufferedWriter(new FileWriter(bidFile, false))) {

            for (int courseId : user.bids.keySet()) {
                String data = courseId + "|" + user.bids.get(courseId).mileage;
                bw.write(data + "\n");
            }

            bw.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    static public void applyRegistration(HashMap<User, ArrayList<Course>> regCourses) throws IOException {

        for (User user : regCourses.keySet()) {
            
            File regFile = new File("data/Users/" + user.userId + "/" + "registrations.txt");
            ArrayList<Course> courses = regCourses.get(user);

            BufferedWriter bw = new BufferedWriter(new FileWriter(regFile, false));

            for (Course course : courses) {
                bw.write(course.toString() + "\n");
            }

            bw.close();
        }
    }
}
