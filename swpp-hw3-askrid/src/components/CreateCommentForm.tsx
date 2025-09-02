import React, { useState } from "react";
import { useDispatch, useSelector } from "react-redux";
import { AppDispatch } from "../store";
import { selectArticle } from "../store/slices/article";
import { createComment } from "../store/slices/comment";

const CreateCommentForm = () => {
  const [content, setContent] = useState<string>("");

  const dispatch = useDispatch<AppDispatch>();
  const { selectedArticle } = useSelector(selectArticle);

  const handleCreateCommentFormSubmit = (e: React.FormEvent<HTMLFormElement>) => {
    e.preventDefault();
    dispatch(createComment({ articleId: selectedArticle!.id, content }));
    setContent("");
  };

  return (
    <form onSubmit={(e) => handleCreateCommentFormSubmit(e)} className="space-y-2">
      <textarea
        id="new-comment-content-input"
        placeholder="What is your opinion?"
        required
        rows={3}
        onChange={(e) => setContent(e.target.value)}
        value={content}
        className="px-6 py-4 resize-none w-full rounded focus:outline-none focus:shadow-inner"
      />
      <button
        id="confirm-create-comment-button"
        type="submit"
        disabled={!content}
        className="block py-2 px-4 w-full rounded bg-slate-800 text-white disabled:opacity-75 shadow-lg"
      >
        Submit
      </button>
    </form>
  );
};

export default CreateCommentForm;
