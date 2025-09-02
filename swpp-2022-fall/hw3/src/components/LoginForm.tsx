import { unwrapResult } from "@reduxjs/toolkit";
import { useState } from "react";
import { useDispatch } from "react-redux";
import { AppDispatch } from "../store";
import { login } from "../store/slices/auth";

const LoginForm = () => {
  const [email, setEmail] = useState("");
  const [password, setPassword] = useState("");

  const dispatch = useDispatch<AppDispatch>();

  const handleLoginFormSubmit = async (e: React.FormEvent<HTMLFormElement>) => {
    e.preventDefault();

    const result = unwrapResult(await dispatch(login({ email, password })));
    if (!result) {
      alert("Email or password is wrong");
    }
  };

  return (
    <form onSubmit={(e) => handleLoginFormSubmit(e)} className="flex flex-col items-center gap-2">
      <input
        id="email-input"
        type="email"
        onChange={(e) => setEmail(e.target.value)}
        placeholder="email"
        className="sm:w-96 w-full p-2 border-2 rounded border-slate-800"
      />
      <input
        id="pw-input"
        type="password"
        onChange={(e) => setPassword(e.target.value)}
        placeholder="password"
        className="sm:w-96 w-full p-2 border-2 rounded border-slate-800"
      />
      <button
        id="login-button"
        type="submit"
        className="sm:w-96 w-full p-2 bg-slate-800 rounded text-white font-semibold"
      >
        Log In
      </button>
    </form>
  );
};

export default LoginForm;
