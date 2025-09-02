import { useDispatch, useSelector } from "react-redux";
import { AppDispatch } from "../store";
import { articleActions, selectArticle } from "../store/slices/article";

const ArticleForm = () => {
  const dispatch = useDispatch<AppDispatch>();
  const { writingArticle } = useSelector(selectArticle);

  const handleTitleChange: React.ChangeEventHandler<HTMLTextAreaElement> = (e) => {
    dispatch(articleActions.writeArticleTitle(e.target.value));
  };

  const handleContentChange: React.ChangeEventHandler<HTMLTextAreaElement> = (e) => {
    dispatch(articleActions.writeArticleContent(e.target.value));
  };

  return (
    <form className="bg-white shadow-lg rounded p-4 space-y-4">
      <textarea
        id="article-title-input"
        placeholder="Title"
        required
        rows={1}
        onChange={(e) => handleTitleChange(e)}
        value={writingArticle.title}
        className="block resize-none w-full rounded focus:outline-none text-3xl p-4 bg-slate-50 focus:shadow-inner"
      />
      <textarea
        id="article-content-input"
        placeholder="Write what you want to read!"
        required
        rows={16}
        onChange={(e) => handleContentChange(e)}
        value={writingArticle.content}
        className="block resize-none w-full rounded focus:outline-none text-lg p-4 bg-slate-50 focus:shadow-inner"
      />
    </form>
  );
};

export default ArticleForm;
