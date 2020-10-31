window.addEventListener('load', () => {

function escapeString(str) {
    return str.replace(/&/g, "&amp;").replace(/>/g, "&gt;").replace(/"/g, "&quot;")
              .replace(/'/g, "&#039;").replace(/</g, "&lt;");
}

(function codeSnippets() {
    function runSnippet(snippetBlock, code) {
        let resultBlock = snippetBlock.querySelector('.snippet-output');
        if(!resultBlock) {
            resultBlock = document.createElement('pre');
            resultBlock.className = 'snippet-style snippet-output';
            snippetBlock.appendChild(resultBlock);
        }

        jstar_api.run(code);

        resultBlock.innerText = jstar_api.out;
        if(jstar_api.err.length > 0) {
            let error = `<span class='snippet-error'>${escapeString(jstar_api.err)}</span>`;
            resultBlock.innerHTML += error;
        }
    }

    // Set up all interactive snippets
    Array.from(document.querySelectorAll('.runnable-snippet')).forEach(function(snippetBlock) {
        let code = snippetBlock.innerHTML;
        
        // Wrap all the snippet elements in a div
        let snippetWrap = document.createElement('div');
        snippetBlock.replaceWith(snippetWrap);
        snippetBlock = snippetWrap;
        
        // Setup the editor
        let editorBlock = document.createElement('div');
        editorBlock.className = 'snippet-style snippet-editor language-jstar';
        snippetBlock.appendChild(editorBlock);

        let editor = CodeJar(editorBlock, Prism.highlightElement);
        editor.updateCode(code);

        // Setup the buttons that control the code in the editor block
        let buttons = document.createElement('div');
        buttons.className = 'snippet-buttons';
        snippetBlock.appendChild(buttons, snippetBlock.firstChild);

        let runButton = document.createElement('button');
        runButton.className = 'fas fa-play snippet-button';
        runButton.title = 'Run the code';
        runButton.addEventListener('click', () => runSnippet(snippetBlock, editor.toString()));
        buttons.appendChild(runButton);

        let resetButton = document.createElement('button');
        resetButton.className = 'fas fa-history snippet-button';
        resetButton.title = 'Undo changes';
        resetButton.addEventListener('click', () => {
            let outputBlock = snippetBlock.querySelector('.snippet-output');
            if(outputBlock) outputBlock.remove();
            editor.updateCode(code);
        });
        buttons.appendChild(resetButton);
    });
})();

});