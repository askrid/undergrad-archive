import { useDispatch, useSelector } from "react-redux";
import { AppDispatch } from "../../store";
import { articleActions, ArticleView, selectArticle } from "../../store/slices/article";

const ArticleViewSelectButtons = () => {
  const dispatch = useDispatch<AppDispatch>();
  const { writingArticleView } = useSelector(selectArticle);

  const handleClick = (view: ArticleView) => {
    dispatch(articleActions.setWritingArticleView(view));
  };

  return (
    <div className="font-semibold shadow-lg">
      <button
        id="write-tab-button"
        disabled={writingArticleView === "WRITE"}
        onClick={() => handleClick("WRITE")}
        className="bg-slate-50 px-6 py-2 rounded-l disabled:brightness-90"
      >
        Write
      </button>
      <button
        id="preview-tab-button"
        disabled={writingArticleView === "PREVIEW"}
        onClick={() => handleClick("PREVIEW")}
        className="bg-slate-50 px-6 py-2 rounded-r disabled:brightness-90"
      >
        Preview
      </button>
    </div>
  );
};

export default ArticleViewSelectButtons;
