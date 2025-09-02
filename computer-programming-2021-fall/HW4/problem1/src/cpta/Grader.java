package cpta;

import cpta.environment.Compiler;
import cpta.environment.Executer;
import cpta.exam.*;
import cpta.exceptions.*;

import java.io.*;
import java.util.*;
import java.util.stream.Collectors;

public class Grader {
    Compiler compiler;
    Executer executer;

    public Grader(Compiler compiler, Executer executer) {
        this.compiler = compiler;
        this.executer = executer;
    }

    public Map<String, Map<String, List<Double>>> gradeSimple(ExamSpec examSpec, String submissionDirPath) {
        Map<String, Map<String, List<Double>>> studentScoreMap = new HashMap<>();

        for (Student student : examSpec.students) {
            Map<String, List<Double>> problemScoreMap = new HashMap<>();

            for (Problem problem : examSpec.problems) {
                List<Double> testScores = new ArrayList<>();

                String problemDirPath = submissionDirPath + student.id + "/" + problem.id + "/";

                try {
                    compiler.compile(problemDirPath + problem.targetFileName);
                } catch (CompileErrorException | InvalidFileTypeException | FileSystemRelatedException e) {
                    e.printStackTrace();
                }

                problem.testCases.sort((x, y) -> x.id.compareTo(y.id));
                String executeTargetPath = problemDirPath + problem.targetFileName.replace(".sugo", ".yo");

                for (TestCase testcase : problem.testCases) {
                    String testInputPath = problem.testCasesDirPath + testcase.inputFileName;
                    String studentOutputPath = problemDirPath + testcase.outputFileName;
                    String correctOutputPath = problem.testCasesDirPath + testcase.outputFileName;

                    try {
                        executer.execute(executeTargetPath, testInputPath, studentOutputPath);
                    } catch (RunTimeErrorException | InvalidFileTypeException | FileSystemRelatedException e) {
                        e.printStackTrace();
                    }

                    BufferedReader br;
                    String studentOutput = "", correctOutput = "";

                    try {
                        br = new BufferedReader(new FileReader(new File(studentOutputPath)));
                        studentOutput = br.lines().collect(Collectors.joining());
                        br = new BufferedReader(new FileReader(new File(correctOutputPath)));
                        correctOutput = br.lines().collect(Collectors.joining());
                        br.close();
                    } catch (IOException e) {
                        e.printStackTrace();
                    }

                    if (studentOutput.compareTo(correctOutput) == 0) {
                        testScores.add(testcase.score);
                    } else {
                        testScores.add(0.0);
                    }
                }

                problemScoreMap.put(problem.id, testScores);
            }

            studentScoreMap.put(student.id, problemScoreMap);
        }

        return studentScoreMap;
    }

    public Map<String, Map<String, List<Double>>> gradeRobust(ExamSpec examSpec, String submissionDirPath) {
        Map<String, Map<String, List<Double>>> studentScoreMap = new HashMap<>();

        File submissionDir = new File(submissionDirPath);

        for (Student student : examSpec.students) {
            Map<String, List<Double>> problemScoreMap = new HashMap<>();

            // get the student directory path; usually it is
            // "{submissionDirPath}/{student.id}/".
            String studentDirPath = "";
            for (File file : submissionDir.listFiles()) {
                if (file.getName().contains(student.id)) {
                    studentDirPath = file.getPath() + "/";
                    break;
                }
            }

            // if the student directory doesn't exist, all the scores are zero.
            if (studentDirPath.isEmpty()) {
                for (Problem problem : examSpec.problems) {
                    List<Double> testScores = Collections.nCopies(problem.testCases.size(), 0.0);
                    problemScoreMap.put(problem.id, testScores);
                }
                studentScoreMap.put(student.id, problemScoreMap);
                continue;
            }

            for (Problem problem : examSpec.problems) {
                List<Double> testScores = new ArrayList<>();

                // get the problem directory path; usually it is
                // "{submissionDirPath}/{student.id}/{problem.id}/".
                String problemDirPath = studentDirPath + problem.id + "/";

                // if the problem directory doesn't exist, all the scores are zero.
                File problemDir = new File(problemDirPath);
                if (!problemDir.exists()) {
                    testScores = Collections.nCopies(problem.testCases.size(), 0.0);
                    problemScoreMap.put(problem.id, testScores);
                    continue;
                }

                // if wrappers exist, copy them to the problem's directory.
                if (problem.wrappersDirPath != null) {
                    File wrappersDir = new File(problem.wrappersDirPath);

                    for (File file : wrappersDir.listFiles()) {
                        File copiedFile = new File(problemDirPath + "/" + file.getName());
                        try {
                            copyFile(file, copiedFile);
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                    }
                }

                // if an additional Dir exists in the problem directory, copy all the files in
                // the additional directory to outside.
                for (File dir : problemDir.listFiles()) {
                    if (dir.isDirectory()) {
                        for (File file : dir.listFiles())
                            try {
                                copyFile(file, new File(problemDirPath + "/" + file.getName()));
                            } catch (IOException e) {
                                e.printStackTrace();
                            }
                        break;
                    }
                }

                // group 4.
                Set<String> problemDirName = new HashSet<>();
                for (File file : problemDir.listFiles()) {
                    problemDirName.add(file.getName());
                }

                boolean foundTargetSourceFile = false, foundTargetCompiledFile = false, foundCompiledOnlyFile = false;

                foundTargetSourceFile = problemDirName.contains(problem.targetFileName);
                foundTargetCompiledFile = problemDirName.contains(problem.targetFileName.replace(".sugo", ".yo"));

                for (String fileName : problemDirName) {
                    if (fileName.endsWith(".yo") && !problemDirName.contains(fileName.replace(".yo", ".sugo"))) {
                        foundCompiledOnlyFile = true;
                        break;
                    }
                }

                if (!foundTargetSourceFile && !foundTargetCompiledFile) {
                    testScores = Collections.nCopies(problem.testCases.size(), 0.0);
                    problemScoreMap.put(problem.id, testScores);
                    continue;
                }

                // compile all the .sugo files.
                try {
                    for (File file : problemDir.listFiles()) {
                        if (file.getName().endsWith(".sugo") && file.isFile())
                            compiler.compile(problemDirPath + file.getName());
                    }
                } catch (CompileErrorException | InvalidFileTypeException | FileSystemRelatedException e) {
                    testScores = Collections.nCopies(problem.testCases.size(), 0.0);
                    problemScoreMap.put(problem.id, testScores);
                    continue;
                }

                problem.testCases.sort((x, y) -> x.id.compareTo(y.id));
                String executeTargetPath = problemDirPath + problem.targetFileName.replace(".sugo", ".yo");

                for (TestCase testcase : problem.testCases) {
                    String testInputPath = problem.testCasesDirPath + testcase.inputFileName;
                    String studentOutputPath = problemDirPath + testcase.outputFileName;
                    String correctOutputPath = problem.testCasesDirPath + testcase.outputFileName;

                    // execute the target file.
                    try {
                        executer.execute(executeTargetPath, testInputPath, studentOutputPath);
                    } catch (RunTimeErrorException | InvalidFileTypeException | FileSystemRelatedException e) {
                        testScores.add(0.0);
                        continue;
                    }

                    BufferedReader br;
                    String studentOutput = "", correctOutput = "";

                    // get the student's output and correct output in string.
                    try {
                        br = new BufferedReader(new FileReader(new File(studentOutputPath)));
                        studentOutput = br.lines().collect(Collectors.joining());
                        br = new BufferedReader(new FileReader(new File(correctOutputPath)));
                        correctOutput = br.lines().collect(Collectors.joining());
                        br.close();
                    } catch (IOException e) {
                        e.printStackTrace();
                        continue;
                    }

                    // consider the problem's judging types.
                    if (problem.judgingTypes != null) {
                        if (problem.judgingTypes.contains(Problem.LEADING_WHITESPACES)) {
                            studentOutput = studentOutput.stripLeading();
                            correctOutput = correctOutput.stripLeading();
                        }
                        if (problem.judgingTypes.contains(Problem.IGNORE_WHITESPACES)) {
                            studentOutput = studentOutput.replaceAll("[ \t]", "");
                            correctOutput = correctOutput.replaceAll("[ \t]", "");
                        }
                        if (problem.judgingTypes.contains(Problem.CASE_INSENSITIVE)) {
                            studentOutput = studentOutput.toLowerCase();
                            correctOutput = correctOutput.toLowerCase();
                        }
                        if (problem.judgingTypes.contains(Problem.IGNORE_SPECIAL_CHARACTERS)) {
                            studentOutput = studentOutput.replaceAll("[^a-zA-Z0-9\\s]", "");
                            correctOutput = correctOutput.replaceAll("[^a-zA-Z0-9\\s]", "");
                        }
                    }

                    // grade the testcase.
                    if (studentOutput.compareTo(correctOutput) == 0) {
                        testScores.add(testcase.score);
                    } else {
                        testScores.add(0.0);
                    }
                }

                if (foundCompiledOnlyFile) {
                    for (int i = 0; i < testScores.size(); i++) {
                        testScores.set(i, testScores.get(i) / 2.0);
                    }
                }

                problemScoreMap.put(problem.id, testScores);
            }

            studentScoreMap.put(student.id, problemScoreMap);
        }

        return studentScoreMap;
    }

    // copy fileToCopy to copiedFile. Overwrite copeidFile if it already exists.
    static void copyFile(File fileToCopy, File copiedFile) throws IOException {
        FileInputStream fis = new FileInputStream(fileToCopy);
        FileOutputStream fos = new FileOutputStream(copiedFile);

        int read;
        byte[] buffer = new byte[512];

        while ((read = fis.read(buffer)) != -1)
            fos.write(buffer, 0, read);

        fis.close();
        fos.close();
    }
}