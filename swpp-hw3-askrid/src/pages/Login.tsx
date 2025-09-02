import LoginForm from "../components/LoginForm";

const Login = () => {
  return (
    <main className="flex flex-col h-screen justify-center">
      <h1 className="font-bold text-center text-4xl mb-6">Sign In Now</h1>
      <LoginForm />
    </main>
  );
};

export default Login;
