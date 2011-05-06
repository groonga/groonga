(function($) {
  $.lookup = function(input, source, columns) {
    var lastq;
    var $input = $(input).attr("autocomplete", "off");
    setTimeout(lookup, 100);
    function lookup() {
      var q = $input.val();
      if (lastq != q) {
        $.getJSON(source+"?callback=?",
                  {query: q,
                   types: 'complete',
                   table: 'item_dictionary',
                   column: 'kana',
                   limit: 25,
                   output_columns: columns},
                  function(json) { displayItems(json[1]["complete"]); });
        lastq = q;
      }
      setTimeout(lookup, 100);
    }
    function displayItems(items) {
      if (items && items.length > 2) {
        var results = $("<dl />");
        items.shift();
        items.shift();
        $.each(items, function(i, val) {
          results.append($("<dt />")
            .append(
              $("<span />")
              .text(val[0])
              .click(function(){
                $(".search").val($(this).text());
                $("#search").submit();
              })));
          results.append($("<dd />")
            .append($("<span />").text(val[1]))
            .append($("<span />").text(val[2]))
          );
        });
        $("#result")
        .empty()
        .append(results);
      }
    }
  }
  $.fn.lookup = function(source, columns) {
    columns = columns || "_key,gene95_desc,edict_desc";
    this.each(function() { new $.lookup(this, source, columns); });
    return this;
  };
})(jQuery);
