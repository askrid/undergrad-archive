package server;

import course.*;
import user.*;
import utils.*;

import java.io.*;
import java.util.*;
import java.util.stream.*;

public class Server {

    HashMap<Integer, Course> courses;
    HashMap<String, User> users;
    HashMap<Integer, ArrayList<User>> coursePriority;
    HashMap<User, ArrayList<Course>> regCourses;
    File coursesDir;
    File usersDir;

    public Server() {

        coursesDir = new File("data/Courses/2020_Spring/");
        usersDir = new File("data/Users/");

        updateCourses();
        updateUsers();
    }

    private void updateCourses() {
        if (coursesDir.isDirectory())
            courses = FileIO.fetchCourses(coursesDir);
    }

    private void updateUsers() {
        if (usersDir.isDirectory())
            users = FileIO.fetchUsers(usersDir);
    }

    public List<Course> search(Map<String, Object> searchConditions, String sortCriteria) {

        Stream<Course> searchStream = courses.values().stream();

        // filter by searchCondtions.
        if (searchConditions != null) {
            if (searchConditions.containsKey("dept")) {
                String dept = (String) searchConditions.get("dept");
                searchStream = searchStream.filter(e -> e.department.compareTo(dept) == 0);
            }
            if (searchConditions.containsKey("ay")) {
                int ay = (int) searchConditions.get("ay");
                searchStream = searchStream.filter(e -> e.academicYear <= ay);
            }
            if (searchConditions.containsKey("name")) {
                String[] keywords = ((String) searchConditions.get("name")).split(" ");
                for (String keyword : keywords) {
                    searchStream = searchStream.filter(e -> Arrays.asList(e.courseName.split(" ")).contains(keyword));
                }
            }
        }

        List<Course> searchList = new ArrayList<>(searchStream.collect(Collectors.toList()));
        searchList.sort((x, y) -> ((Integer) x.courseId).compareTo((Integer) y.courseId));

        // sort by sortCirteria
        if (sortCriteria != null) {
            if (sortCriteria.compareTo("name") == 0) {
                searchList.sort((x, y) -> x.courseName.compareTo(y.courseName));
            }
            if (sortCriteria.compareTo("dept") == 0) {
                searchList.sort((x, y) -> x.department.compareTo(y.department));
            }
            if (sortCriteria.compareTo("ay") == 0) {
                searchList.sort((x, y) -> ((Integer) x.academicYear).compareTo((Integer) y.academicYear));
            }
        }

        return searchList;
    }

    public int bid(int courseId, int mileage, String userId) {

        updateUsers();

        // error handling.
        if (userId == null || !users.containsKey(userId))
            return ErrorCode.USERID_NOT_FOUND;

        User user = users.get(userId);

        if (!courses.containsKey(courseId))
            return ErrorCode.NO_COURSE_ID;

        if (mileage < 0)
            return ErrorCode.NEGATIVE_MILEAGE;

        if (!user.hasBidFile())
            return ErrorCode.IO_ERROR;

        if (mileage > Config.MAX_MILEAGE_PER_COURSE)
            return ErrorCode.OVER_MAX_COURSE_MILEAGE;

        if (user.bids.containsKey(courseId)) {
            if (mileage - user.bids.get(courseId).mileage + user.usedMileage > Config.MAX_MILEAGE)
                return ErrorCode.OVER_MAX_MILEAGE;
        } else {
            if (mileage + user.usedMileage > Config.MAX_MILEAGE)
                return ErrorCode.OVER_MAX_MILEAGE;
        }

        if (user.courseNumber == Config.MAX_COURSE_NUMBER && !user.bids.containsKey(courseId) && mileage != 0)
            return ErrorCode.OVER_MAX_COURSE_NUMBER;

        user.bid(courseId, mileage);
        FileIO.applyBids(user);

        return ErrorCode.SUCCESS;
    }

    public Pair<Integer, List<Bidding>> retrieveBids(String userId) {

        updateUsers();

        if (userId == null || !users.containsKey(userId))
            return new Pair<>(ErrorCode.USERID_NOT_FOUND, new ArrayList<Bidding>());

        User user = users.get(userId);

        if (!user.hasBidFile())
            return new Pair<>(ErrorCode.IO_ERROR, new ArrayList<Bidding>());

        ArrayList<Bidding> bids = new ArrayList<>(user.bids.values());

        return new Pair<>(ErrorCode.SUCCESS, bids);
    }

    public boolean confirmBids() {

        updateUsers();

        coursePriority = new HashMap<>();
        regCourses = new HashMap<>();

        // coursePriority
        for (Course course : courses.values()) {
            coursePriority.put(course.courseId, new ArrayList<User>());
        }

        for (User user : users.values()) {
            if (user.hasBidFile()) {
                for (Bidding bid : user.bids.values()) {
                    coursePriority.get(bid.courseId).add(user);
                }
            } else {
                return false;
            }
        }

        for (int courseId : coursePriority.keySet()) {
            ArrayList<User> priorityList = coursePriority.get(courseId);

            priorityList.sort((x, y) -> x.userId.compareTo(y.userId));
            priorityList.sort((x, y) -> ((Integer) x.usedMileage).compareTo((Integer) y.usedMileage));
            priorityList.sort((x, y) -> ((Integer) y.bids.get(courseId).mileage)
                    .compareTo((Integer) x.bids.get(courseId).mileage));
        }

        // regCourses
        for (User user : users.values()) {
            regCourses.put(user, new ArrayList<Course>());
        }

        for (int courseId : coursePriority.keySet()) {
            int cutline = courses.get(courseId).quota;
            ArrayList<User> biddedUsers = coursePriority.get(courseId);

            for (int i = 0; i < biddedUsers.size(); i++) {
                if (i >= cutline)
                    break;

                User user = biddedUsers.get(i);
                regCourses.get(user).add(courses.get(courseId));
            }
        }

        // remove all the bid informations
        for (User user : users.values()) {
            user.usedMileage = 0;
            user.courseNumber = 0;
            user.bids = new HashMap<>();

            FileIO.applyBids(user);
        }

        try {
            FileIO.applyRegistration(regCourses);
        } catch (IOException e) {
            return false;
        }

        return true;
    }

    public Pair<Integer, List<Course>> retrieveRegisteredCourse(String userId) {

        if (userId == null || !users.containsKey(userId))
            return new Pair<>(ErrorCode.USERID_NOT_FOUND, new ArrayList<Course>());

        User user = users.get(userId);

        try {
            ArrayList<Course> regList = regCourses.get(user);
            return new Pair<>(ErrorCode.SUCCESS, regList);
        } catch (Exception e) {
            return new Pair<>(ErrorCode.IO_ERROR, new ArrayList<Course>());
        }
    }
}