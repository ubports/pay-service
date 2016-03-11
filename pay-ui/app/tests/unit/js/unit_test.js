.pragma library

// Find an object with the given name in the children tree of "obj"
function findChild(obj,objectName) {
    var childs = new Array(0);
    childs.push(obj)
    while (childs.length > 0) {
        if (childs[0].objectName == objectName) {
            return childs[0]
        }
        for (var i in childs[0].children) {
            childs.push(childs[0].children[i])
        }
        childs.splice(0, 1);
    }
    return undefined;
}
