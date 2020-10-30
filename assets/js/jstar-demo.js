window.addEventListener("load", () => {
    // Demo elements and helper functions
    let input = document.querySelector("#input");
    let output = document.querySelector("#output");
    let error_label = document.querySelector("#error-label");
    let editor = CodeJar(input, withLineNumbers(Prism.highlightElement), { 'tab' : ' '.repeat(4) });

    function setErrorLabel(message) {
        error_label.innerText = message;
        error_label.setAttribute('class', 'label label-red');
    }

    function clearErrorLabel() {
        error_label.innerText = '';
        error_label.setAttribute('class', '');
    }

    function clearOutput() {
        output.innerText = "";
    }

    function addOutput(out) {
        output.innerText += out;
    }

    function addErrorOutput(err) {
        err = err.replace(/&/g, "&amp;").replace(/>/g, "&gt;").replace(/"/g, "&quot;")
                 .replace(/'/g, "&#039;").replace(/</g, "&lt;");
        output.innerHTML += `<span class="error">${err}</span>`;
    }

    function setEditorCode(source) {
        clearOutput();
        addOutput('[...]');
        clearErrorLabel();
        editor.updateCode(source);
    }

    // Button functionalities
    let run_btn = document.querySelector('#run');
    run_btn.addEventListener('click', () => {
        clearErrorLabel();
        clearOutput();

        let res = jstar_api.run(editor.toString());

        if(res == 1) {
            setErrorLabel('Syntax Error');
        } else if(res == 2) {
            setErrorLabel('Compilation Error');
        } else if(res == 3) {
            setErrorLabel('Runtime Error');
        }

        addOutput(jstar_api.out);
        addErrorOutput(jstar_api.err);
    });

    // Example buttons
    let hello_world = document.querySelector('#hello-world');
    hello_world.addEventListener('click', () => {
        setEditorCode(`print('Hello, World!')`);
    });

    let loop = document.querySelector('#loop');
    loop.addEventListener('click', () => {
        setEditorCode(
`for var i = 0; i < 10; i += 1 do
    print('Iteration {0}' % i)
end`
        );
    });

    let quick_sort = document.querySelector('#quick-sort');
    quick_sort.addEventListener('click', () => {
        setEditorCode(
`fun partition(list, low, high)
    var pivot = list[high]
    var i = low
    for var j = low; j < high; j += 1 do
        if list[j] <= pivot then
            list[i], list[j] = list[j], list[i]
            i += 1
        end
    end
    list[i], list[high] = list[high], list[i]
    return i
end

fun quickSort(list, low, high)
    if low < high then
        var p = partition(list, low, high)
        quickSort(list, low, p - 1)
        quickSort(list, p + 1, high)
    end
end

var list = [9, 1, 36, 37, 67, 45, 11, 27, 3, 5]
quickSort(list, 0, #list - 1)
print(list)`
        );
    });

    let regex = document.querySelector('#regex');
    regex.addEventListener('click', () => {
        setEditorCode(
`import re

var message = '{lang} on {platform}!'
var formatted = re.gsub(message, '{(%a+)}', fun(arg)
    return { 
        'lang' : 'J*', 'platform' : 'the Web'
    }[arg]
end)

print(formatted)`
        );
    });
});
