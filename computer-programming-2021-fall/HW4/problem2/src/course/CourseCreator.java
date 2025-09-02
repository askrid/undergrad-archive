package course;

import java.io.*;

public class CourseCreator {
    public int courseId;
    public String courseName;
    public String college;
    public String department;
    public int academicYear;
    public int credit;
    public String academicDegree;
    public String instructor;
    public int quota;
    public String location;

    public CourseCreator(String collegeName, int courseId, File courseFile) {
        parseArgs(collegeName, courseId, courseFile);
    }

    private void parseArgs(String collegeName, int courseId, File courseFile) {
        try (BufferedReader br = new BufferedReader(new FileReader(courseFile))) {

            String[] args = br.readLine().split("\\|");

            this.courseId = courseId;
            this.courseName = args[3];
            this.college = collegeName;
            this.department = args[0];
            this.academicYear = Integer.parseInt(args[2]);
            this.credit = Integer.parseInt(args[4]);
            this.academicDegree = args[1];
            this.instructor = args[6];
            this.quota = Integer.parseInt(args[7]);
            this.location = args[5];

        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public Course create() {
        return new Course(courseId, college, department, academicDegree, academicYear, courseName, credit, location,
                instructor, quota);
    }
}
