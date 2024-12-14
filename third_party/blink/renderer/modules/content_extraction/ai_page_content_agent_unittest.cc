// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/content_extraction/ai_page_content_agent.h"

#include "mojo/public/cpp/test_support/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/frame_test_helpers.h"
#include "third_party/blink/renderer/core/html/html_collection.h"
#include "third_party/blink/renderer/core/html/html_iframe_element.h"
#include "third_party/blink/renderer/core/layout/layout_object.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/platform/testing/task_environment.h"
#include "third_party/blink/renderer/platform/testing/unit_test_helpers.h"
#include "third_party/blink/renderer/platform/testing/url_test_helpers.h"

namespace blink {
namespace {

constexpr gfx::Size kWindowSize{1000, 1000};
constexpr char kSmallImage[] =
    "data:image/jpeg;base64,/9j/4AAQSkZJRgABAQAAAQABAAD/"
    "2wBDAAMCAgICAgMCAgIDAwMDBAYEBAQEBAgGBgUGCQgKCgkICQkKDA8MCgsOCwkJDRENDg8QEB"
    "EQCgwSExIQEw8QEBD/"
    "2wBDAQMDAwQDBAgEBAgQCwkLEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEB"
    "AQEBAQEBAQEBAQEBD/wAARCAABAAEDASIAAhEBAxEB/"
    "8QAHwAAAQUBAQEBAQEAAAAAAAAAAAECAwQFBgcICQoL/"
    "8QAtRAAAgEDAwIEAwUFBAQAAAF9AQIDAAQRBRIhMUEGE1FhByJxFDKBkaEII0KxwRVS0fAkM2J"
    "yggkKFhcYGRolJicoKSo0NTY3ODk6Q0RFRkdISUpTVFVWV1hZWmNkZWZnaGlqc3R1dnd4eXqDh"
    "IWGh4iJipKTlJWWl5iZmqKjpKWmp6ipqrKztLW2t7i5usLDxMXGx8jJytLT1NXW19jZ2uHi4+"
    "Tl5ufo6erx8vP09fb3+Pn6/8QAHwEAAwEBAQEBAQEBAQAAAAAAAAECAwQFBgcICQoL/"
    "8QAtREAAgECBAQDBAcFBAQAAQJ3AAECAxEEBSExBhJBUQdhcRMiMoEIFEKRobHBCSMzUvAVYnL"
    "RChYkNOEl8RcYGRomJygpKjU2Nzg5OkNERUZHSElKU1RVVldYWVpjZGVmZ2hpanN0dXZ3eHl6g"
    "oOEhYaHiImKkpOUlZaXmJmaoqOkpaanqKmqsrO0tba3uLm6wsPExcbHyMnK0tPU1dbX2Nna4uP"
    "k5ebn6Onq8vP09fb3+Pn6/9oADAMBAAIRAxEAPwD+/iiiigD/2Q==";

class AIPageContentAgentTest : public testing::Test {
 public:
  AIPageContentAgentTest() = default;
  ~AIPageContentAgentTest() override = default;

  void SetUp() override {
    helper_.Initialize();
    helper_.Resize(kWindowSize);
    ASSERT_TRUE(helper_.LocalMainFrame());
  }

 protected:
  test::TaskEnvironment task_environment_;
  frame_test_helpers::WebViewHelper helper_;
};

TEST_F(AIPageContentAgentTest, Basic) {
  frame_test_helpers::LoadHTMLString(
      helper_.LocalMainFrame(),
      "<body>"
      "  <style>"
      "    div {"
      "      position: absolute;"
      "      top: -10px;"
      "      left: -20px;"
      "    }"
      "  </style>"
      "  <div>text</div>"
      "</body>",
      url_test_helpers::ToKURL("http://foobar.com"));

  auto* agent = AIPageContentAgent::GetOrCreateForTesting(
      *helper_.LocalMainFrame()->GetFrame()->GetDocument());
  ASSERT_TRUE(agent);

  auto content = agent->GetAIPageContentSync();
  ASSERT_TRUE(content);
  ASSERT_TRUE(content->root_node);

  const auto& root = *content->root_node;
  EXPECT_TRUE(root.children_nodes.empty());

  const auto& attributes = *root.content_attributes;
  // One for root itself and one for the text content.
  EXPECT_EQ(attributes.dom_node_ids.size(), 2u);
  EXPECT_TRUE(attributes.common_ancestor_dom_node_id.has_value());

  EXPECT_EQ(attributes.attribute_type,
            mojom::blink::AIPageContentAttributeType::kRoot);

  ASSERT_TRUE(attributes.geometry);
  EXPECT_EQ(attributes.geometry->outer_bounding_box, gfx::Rect(kWindowSize));
  EXPECT_EQ(attributes.geometry->visible_bounding_box, gfx::Rect(kWindowSize));

  ASSERT_EQ(attributes.text_info.size(), 1u);
  const auto& text_info = *attributes.text_info[0];
  EXPECT_EQ(text_info.text_content, "text");
  EXPECT_EQ(text_info.text_bounding_box.x(), -20);
  EXPECT_EQ(text_info.text_bounding_box.y(), -10);
}

TEST_F(AIPageContentAgentTest, Image) {
  frame_test_helpers::LoadHTMLString(
      helper_.LocalMainFrame(),
      "<body>"
      "  <style>"
      "    img {"
      "      position: absolute;"
      "      top: -10px;"
      "      left: -20px;"
      "      width: 30px;"
      "      height: 40px;"
      "    }"
      "  </style>"
      "  <img alt=missing></img>"
      "</body>",
      url_test_helpers::ToKURL("http://foobar.com"));
  auto& document = *helper_.LocalMainFrame()->GetFrame()->GetDocument();
  document.getElementsByTagName(AtomicString("img"))
      ->item(0)
      ->setAttribute(html_names::kSrcAttr, AtomicString(kSmallImage));

  auto* agent = AIPageContentAgent::GetOrCreateForTesting(document);
  ASSERT_TRUE(agent);

  auto content = agent->GetAIPageContentSync();
  ASSERT_TRUE(content);
  ASSERT_TRUE(content->root_node);

  const auto& root = *content->root_node;
  EXPECT_TRUE(root.children_nodes.empty());

  const auto& attributes = *root.content_attributes;
  // One for root itself and one for the image content.
  EXPECT_EQ(attributes.dom_node_ids.size(), 2u);

  ASSERT_EQ(attributes.image_info.size(), 1u);
  const auto& image_info = *attributes.image_info[0];
  EXPECT_EQ(image_info.image_caption, "missing");
  EXPECT_EQ(image_info.image_bounding_box, gfx::Rect(-20, -10, 30, 40));
}

TEST_F(AIPageContentAgentTest, ImageNoAltText) {
  frame_test_helpers::LoadHTMLString(
      helper_.LocalMainFrame(),
      base::StringPrintf("<body>"
                         "  <style>"
                         "    div::before {"
                         "      content: url(%s);"
                         "    }"
                         "  </style>"
                         "  <div>text</div>"
                         "</body>",
                         kSmallImage),
      url_test_helpers::ToKURL("http://foobar.com"));
  auto& document = *helper_.LocalMainFrame()->GetFrame()->GetDocument();

  auto* agent = AIPageContentAgent::GetOrCreateForTesting(document);
  auto page_content = agent->GetAIPageContentSync();

  mojom::blink::AIPageContentPtr output;
  ASSERT_TRUE(mojo::test::SerializeAndDeserialize<mojom::blink::AIPageContent>(
      page_content, output));
}

TEST_F(AIPageContentAgentTest, Headings) {
  frame_test_helpers::LoadHTMLString(
      helper_.LocalMainFrame(),
      "<body>"
      "  <h1>Heading 1</h1>"
      "  <h2>Heading 2</h2>"
      "  <h3>Heading 3</h3>"
      "</body>",
      url_test_helpers::ToKURL("http://foobar.com"));

  auto* agent = AIPageContentAgent::GetOrCreateForTesting(
      *helper_.LocalMainFrame()->GetFrame()->GetDocument());
  ASSERT_TRUE(agent);

  auto content = agent->GetAIPageContentSync();
  ASSERT_TRUE(content);
  ASSERT_TRUE(content->root_node);

  const auto& root = *content->root_node;
  ASSERT_EQ(root.children_nodes.size(), 3u);

  const auto& heading1 = *root.children_nodes[0]->content_attributes;
  EXPECT_EQ(heading1.attribute_type,
            mojom::blink::AIPageContentAttributeType::kHeading);
  ASSERT_EQ(heading1.text_info.size(), 1u);
  EXPECT_EQ(heading1.text_info[0]->text_content, "Heading 1");

  const auto& heading2 = *root.children_nodes[1]->content_attributes;
  EXPECT_EQ(heading2.attribute_type,
            mojom::blink::AIPageContentAttributeType::kHeading);
  ASSERT_EQ(heading2.text_info.size(), 1u);
  EXPECT_EQ(heading2.text_info[0]->text_content, "Heading 2");

  const auto& heading3 = *root.children_nodes[2]->content_attributes;
  EXPECT_EQ(heading3.attribute_type,
            mojom::blink::AIPageContentAttributeType::kHeading);
  ASSERT_EQ(heading3.text_info.size(), 1u);
  EXPECT_EQ(heading3.text_info[0]->text_content, "Heading 3");
}

TEST_F(AIPageContentAgentTest, Paragraph) {
  frame_test_helpers::LoadHTMLString(
      helper_.LocalMainFrame(),
      "<body>"
      "  <style>"
      "    p {"
      "      position: fixed;"
      "      top: -10px;"
      "      left: -20px;"
      "      width: 200px;"
      "      height: 40px;"
      "      margin: 0;"
      "    }"
      "  </style>"
      "  <p>text inside paragraph</p>"
      "</body>",
      url_test_helpers::ToKURL("http://foobar.com"));

  auto* agent = AIPageContentAgent::GetOrCreateForTesting(
      *helper_.LocalMainFrame()->GetFrame()->GetDocument());
  ASSERT_TRUE(agent);

  auto content = agent->GetAIPageContentSync();
  ASSERT_TRUE(content);
  ASSERT_TRUE(content->root_node);

  const auto& root = *content->root_node;
  ASSERT_EQ(root.children_nodes.size(), 1u);

  const auto& paragraph = *root.children_nodes[0]->content_attributes;
  EXPECT_EQ(paragraph.attribute_type,
            mojom::blink::AIPageContentAttributeType::kParagraph);
  EXPECT_EQ(paragraph.geometry->outer_bounding_box,
            gfx::Rect(-20, -10, 200, 40));
  EXPECT_EQ(paragraph.geometry->visible_bounding_box, gfx::Rect(0, 0, 180, 30));

  ASSERT_EQ(paragraph.text_info.size(), 1u);
  EXPECT_EQ(paragraph.text_info[0]->text_content, "text inside paragraph");
}

TEST_F(AIPageContentAgentTest, Lists) {
  frame_test_helpers::LoadHTMLString(
      helper_.LocalMainFrame(),
      "<body>"
      "  <ul>"
      "    <li>Item 1</li>"
      "    <li>Item 2</li>"
      "  </ul>"
      "  <ol>"
      "    <li>Step 1</li>"
      "    <li>Step 2</li>"
      "  </ol>"
      "  <dl>"
      "    <dt>Detail 1 title</dt>"
      "    <dd>Detail 1 description</dd>"
      "    <dt>Detail 2 title</dt>"
      "    <dd>Detail 2 description</dd>"
      "  </dl>"
      "</body>",
      url_test_helpers::ToKURL("http://foobar.com"));

  auto* agent = AIPageContentAgent::GetOrCreateForTesting(
      *helper_.LocalMainFrame()->GetFrame()->GetDocument());
  ASSERT_TRUE(agent);

  auto content = agent->GetAIPageContentSync();
  ASSERT_TRUE(content);
  ASSERT_TRUE(content->root_node);

  const auto& root = *content->root_node;
  ASSERT_EQ(root.children_nodes.size(), 3u);

  const auto& ul = *root.children_nodes[0]->content_attributes;
  EXPECT_EQ(ul.attribute_type,
            mojom::blink::AIPageContentAttributeType::kUnorderedList);
  ASSERT_EQ(ul.text_info.size(), 2u);
  EXPECT_EQ(ul.text_info[0]->text_content, "Item 1");
  EXPECT_EQ(ul.text_info[1]->text_content, "Item 2");

  const auto& ol = *root.children_nodes[1]->content_attributes;
  EXPECT_EQ(ol.attribute_type,
            mojom::blink::AIPageContentAttributeType::kOrderedList);
  ASSERT_EQ(ol.text_info.size(), 2u);
  EXPECT_EQ(ol.text_info[0]->text_content, "Step 1");
  EXPECT_EQ(ol.text_info[1]->text_content, "Step 2");

  const auto& dl = *root.children_nodes[2]->content_attributes;
  EXPECT_EQ(dl.attribute_type,
            mojom::blink::AIPageContentAttributeType::kUnorderedList);
  ASSERT_EQ(dl.text_info.size(), 4u);
  EXPECT_EQ(dl.text_info[0]->text_content, "Detail 1 title");
  EXPECT_EQ(dl.text_info[1]->text_content, "Detail 1 description");
  EXPECT_EQ(dl.text_info[2]->text_content, "Detail 2 title");
  EXPECT_EQ(dl.text_info[3]->text_content, "Detail 2 description");
}

TEST_F(AIPageContentAgentTest, IFrameWithContent) {
  frame_test_helpers::LoadHTMLString(
      helper_.LocalMainFrame(),
      "<body>"
      "  <iframe src='about:blank'></iframe>"
      "</body>",
      url_test_helpers::ToKURL("http://foobar.com"));

  auto* iframe_element =
      To<HTMLIFrameElement>(helper_.LocalMainFrame()
                                ->GetFrame()
                                ->GetDocument()
                                ->getElementsByTagName(AtomicString("iframe"))
                                ->item(0));
  ASSERT_TRUE(iframe_element);

  // Access the iframe's document and set some content
  auto* iframe_doc = iframe_element->contentDocument();
  ASSERT_TRUE(iframe_doc);

  iframe_doc->body()->setInnerHTML("<body>inside iframe</body>");

  auto* agent = AIPageContentAgent::GetOrCreateForTesting(
      *helper_.LocalMainFrame()->GetFrame()->GetDocument());
  ASSERT_TRUE(agent);

  auto content = agent->GetAIPageContentSync();
  ASSERT_TRUE(content);
  ASSERT_TRUE(content->root_node);

  const auto& root = *content->root_node;
  ASSERT_EQ(root.children_nodes.size(), 1u);

  const auto& iframe = *root.children_nodes[0];
  const auto& iframe_attributes = *iframe.content_attributes;

  EXPECT_EQ(iframe_attributes.attribute_type,
            mojom::blink::AIPageContentAttributeType::kIframe);

  const auto& iframe_root = iframe.children_nodes[0];
  const auto& iframe_root_attributes = *iframe_root->content_attributes;

  ASSERT_EQ(iframe_root_attributes.text_info.size(), 1u);
  EXPECT_EQ(iframe_root_attributes.text_info[0]->text_content, "inside iframe");
}

TEST_F(AIPageContentAgentTest, NoLayoutElement) {
  frame_test_helpers::LoadHTMLString(
      helper_.LocalMainFrame(),
      "<body>"
      "  <div style='display: none;'>Hidden Content</div>"
      "</body>",
      url_test_helpers::ToKURL("http://foobar.com"));

  auto* agent = AIPageContentAgent::GetOrCreateForTesting(
      *helper_.LocalMainFrame()->GetFrame()->GetDocument());
  ASSERT_TRUE(agent);

  auto content = agent->GetAIPageContentSync();
  ASSERT_TRUE(content);
  ASSERT_TRUE(content->root_node);

  const auto& root = *content->root_node;
  EXPECT_TRUE(root.children_nodes.empty());
  EXPECT_TRUE(root.content_attributes->text_info.empty());
}

TEST_F(AIPageContentAgentTest, VisibilityHidden) {
  frame_test_helpers::LoadHTMLString(
      helper_.LocalMainFrame(),
      "<body>"
      "  <div style='visibility: hidden;'>Hidden Content</div>"
      "</body>",
      url_test_helpers::ToKURL("http://foobar.com"));

  auto* agent = AIPageContentAgent::GetOrCreateForTesting(
      *helper_.LocalMainFrame()->GetFrame()->GetDocument());
  ASSERT_TRUE(agent);

  auto content = agent->GetAIPageContentSync();
  ASSERT_TRUE(content);
  ASSERT_TRUE(content->root_node);

  const auto& root = *content->root_node;
  EXPECT_TRUE(root.children_nodes.empty());
  EXPECT_TRUE(root.content_attributes->text_info.empty());
}

TEST_F(AIPageContentAgentTest, TextSize) {
  frame_test_helpers::LoadHTMLString(
      helper_.LocalMainFrame(),
      "<body>"
      "  <h1>Extra large text</h1>"
      "  <h2>Large text</h2>"
      "  <p>Regular text</p>"
      "  <h6>Small text</h6>"
      "  <p style='font-size: 0.25em;'>Extra small text</p>"
      "</body>",
      url_test_helpers::ToKURL("http://foobar.com"));

  auto* agent = AIPageContentAgent::GetOrCreateForTesting(
      *helper_.LocalMainFrame()->GetFrame()->GetDocument());
  ASSERT_TRUE(agent);

  auto content = agent->GetAIPageContentSync();
  ASSERT_TRUE(content);
  ASSERT_TRUE(content->root_node);

  const auto& root = *content->root_node;
  ASSERT_EQ(root.children_nodes.size(), 5u);

  const auto& xl_text = *root.children_nodes[0]->content_attributes;
  ASSERT_EQ(xl_text.text_info.size(), 1u);
  EXPECT_EQ(xl_text.text_info[0]->text_style->text_size,
            mojom::blink::AIPageContentTextSize::kXL);

  const auto& l_text = *root.children_nodes[1]->content_attributes;
  ASSERT_EQ(l_text.text_info.size(), 1u);
  EXPECT_EQ(l_text.text_info[0]->text_style->text_size,
            mojom::blink::AIPageContentTextSize::kL);

  const auto& m_text = *root.children_nodes[2]->content_attributes;
  ASSERT_EQ(m_text.text_info.size(), 1u);
  EXPECT_EQ(m_text.text_info[0]->text_style->text_size,
            mojom::blink::AIPageContentTextSize::kM);

  const auto& s_text = *root.children_nodes[3]->content_attributes;
  ASSERT_EQ(s_text.text_info.size(), 1u);
  EXPECT_EQ(s_text.text_info[0]->text_style->text_size,
            mojom::blink::AIPageContentTextSize::kS);

  const auto& xs_text = *root.children_nodes[4]->content_attributes;
  ASSERT_EQ(xs_text.text_info.size(), 1u);
  EXPECT_EQ(xs_text.text_info[0]->text_style->text_size,
            mojom::blink::AIPageContentTextSize::kXS);
}

TEST_F(AIPageContentAgentTest, TextEmphasis) {
  frame_test_helpers::LoadHTMLString(
      helper_.LocalMainFrame(),
      "<body>"
      "<p>Regular text"
      "<b>Bolded text</b>"
      "<i>Italicized text</i>"
      "<u>Underlined text</u>"
      "<sub>Subscript text</sub>"
      "<sup>Superscript text</sup>"
      "<em>Emphasized text</em>"
      "<strong>Strong text</strong>"
      "</p>"
      "</body>",
      url_test_helpers::ToKURL("http://foobar.com"));

  auto* agent = AIPageContentAgent::GetOrCreateForTesting(
      *helper_.LocalMainFrame()->GetFrame()->GetDocument());
  ASSERT_TRUE(agent);

  auto content = agent->GetAIPageContentSync();
  ASSERT_TRUE(content);
  ASSERT_TRUE(content->root_node);

  const auto& root = *content->root_node;
  ASSERT_EQ(root.children_nodes.size(), 1u);
  const auto& text = *root.children_nodes[0]->content_attributes;
  ASSERT_EQ(text.text_info.size(), 8u);

  EXPECT_EQ(text.text_info[0]->text_content, "Regular text");
  EXPECT_FALSE(text.text_info[0]->text_style->has_emphasis);

  EXPECT_EQ(text.text_info[1]->text_content, "Bolded text");
  EXPECT_TRUE(text.text_info[1]->text_style->has_emphasis);

  EXPECT_EQ(text.text_info[2]->text_content, "Italicized text");
  EXPECT_TRUE(text.text_info[2]->text_style->has_emphasis);

  EXPECT_EQ(text.text_info[3]->text_content, "Underlined text");
  EXPECT_TRUE(text.text_info[3]->text_style->has_emphasis);

  EXPECT_EQ(text.text_info[4]->text_content, "Subscript text");
  EXPECT_TRUE(text.text_info[4]->text_style->has_emphasis);

  EXPECT_EQ(text.text_info[5]->text_content, "Superscript text");
  EXPECT_TRUE(text.text_info[5]->text_style->has_emphasis);

  EXPECT_EQ(text.text_info[6]->text_content, "Emphasized text");
  EXPECT_TRUE(text.text_info[6]->text_style->has_emphasis);

  EXPECT_EQ(text.text_info[7]->text_content, "Strong text");
  EXPECT_TRUE(text.text_info[7]->text_style->has_emphasis);
}

TEST_F(AIPageContentAgentTest, Table) {
  frame_test_helpers::LoadHTMLString(
      helper_.LocalMainFrame(),
      "<body>"
      "  <table>"
      "    <caption>Table caption</caption>"
      "    <thead>"
      "      <th colspan='2'>Header</th>"
      "    </thead>"
      "    <tr>"
      "      <td>Row 1 Column 1</td>"
      "      <td>Row 1 Column 2</td>"
      "    </tr>"
      "    <tr>"
      "      <td>Row 2 Column 1</td>"
      "      <td>Row 2 Column 2</td>"
      "    </tr>"
      "    <tfoot>"
      "      <td>Footer 1</td>"
      "      <td>Footer 2</td>"
      "    </tfoot>"
      "  </table>"
      "</body>",
      url_test_helpers::ToKURL("http://foobar.com"));

  auto* agent = AIPageContentAgent::GetOrCreateForTesting(
      *helper_.LocalMainFrame()->GetFrame()->GetDocument());
  ASSERT_TRUE(agent);

  auto content = agent->GetAIPageContentSync();
  ASSERT_TRUE(content);
  ASSERT_TRUE(content->root_node);

  const auto& root = *content->root_node;
  ASSERT_EQ(root.children_nodes.size(), 1u);

  const auto& table = *root.children_nodes[0]->content_attributes;
  EXPECT_EQ(table.attribute_type,
            mojom::blink::AIPageContentAttributeType::kTable);
  ASSERT_TRUE(table.table_data);

  EXPECT_EQ(table.table_data->table_name, "Table caption");

  const auto& header_rows = table.table_data->header_rows;
  EXPECT_EQ(header_rows.size(), 1u);

  const auto& header_row = header_rows[0]->cells;
  EXPECT_EQ(header_row.size(), 1u);
  EXPECT_EQ(header_row[0]->content_attributes->text_info[0]->text_content,
            "Header");

  const auto& body_rows = table.table_data->body_rows;
  EXPECT_EQ(body_rows.size(), 2u);

  const auto& row_1 = body_rows[0]->cells;
  EXPECT_EQ(row_1.size(), 2u);
  EXPECT_EQ(row_1[0]->content_attributes->text_info[0]->text_content,
            "Row 1 Column 1");
  EXPECT_EQ(row_1[1]->content_attributes->text_info[0]->text_content,
            "Row 1 Column 2");

  const auto& row_2 = body_rows[1]->cells;
  EXPECT_EQ(row_2.size(), 2u);
  EXPECT_EQ(row_2[0]->content_attributes->text_info[0]->text_content,
            "Row 2 Column 1");
  EXPECT_EQ(row_2[1]->content_attributes->text_info[0]->text_content,
            "Row 2 Column 2");

  const auto& footer_rows = table.table_data->footer_rows;
  EXPECT_EQ(footer_rows.size(), 1u);

  const auto& footer_row = footer_rows[0]->cells;
  EXPECT_EQ(footer_row.size(), 2u);
  EXPECT_EQ(footer_row[0]->content_attributes->text_info[0]->text_content,
            "Footer 1");
  EXPECT_EQ(footer_row[1]->content_attributes->text_info[0]->text_content,
            "Footer 2");
}

TEST_F(AIPageContentAgentTest, TableMadeWithCss) {
  frame_test_helpers::LoadHTMLString(
      helper_.LocalMainFrame(),
      "<body>"
      "    <style>"
      "        .table {"
      "            display: table;"
      "            border-collapse: collapse;"
      "            width: 100%;"
      "        }"
      "        .row {"
      "            display: table-row;"
      "        }"
      "        .cell {"
      "            display: table-cell;"
      "            border: 1px solid #000;"
      "            padding: 8px;"
      "            text-align: center;"
      "        }"
      "        .header {"
      "            background-color: #f4f4f4;"
      "            font-weight: bold;"
      "        }"
      "    </style>"
      "    <div class='table'>"
      //       Header Rows
      "        <div class='row header'>"
      "            <div class='cell' colspan='2'>Personal Info</div>"
      "            <div class='cell' colspan='2'>Contact Info</div>"
      "        </div>"
      "        <div class='row header'>"
      "            <div class='cell'>Name</div>"
      "            <div class='cell'>Age</div>"
      "            <div class='cell'>Email</div>"
      "            <div class='cell'>Phone</div>"
      "        </div>"
      //       Body Rows
      "        <div class='row'>"
      "            <div class='cell'>John Doe</div>"
      "            <div class='cell'>30</div>"
      "            <div class='cell'>john.doe@example.com</div>"
      "            <div class='cell'>123-456-7890</div>"
      "        </div>"
      "        <div class='row'>"
      "            <div class='cell'>Jane Smith</div>"
      "            <div class='cell'>28</div>"
      "            <div class='cell'>jane.smith@example.com</div>"
      "            <div class='cell'>987-654-3210</div>"
      "        </div>"
      "    </div>"
      "</body>",
      url_test_helpers::ToKURL("http://foobar.com"));

  auto* agent = AIPageContentAgent::GetOrCreateForTesting(
      *helper_.LocalMainFrame()->GetFrame()->GetDocument());
  ASSERT_TRUE(agent);

  auto content = agent->GetAIPageContentSync();
  ASSERT_TRUE(content);
  ASSERT_TRUE(content->root_node);

  const auto& root = *content->root_node;
  ASSERT_EQ(root.children_nodes.size(), 1u);

  const auto& table = *root.children_nodes[0]->content_attributes;
  EXPECT_EQ(table.attribute_type,
            mojom::blink::AIPageContentAttributeType::kTable);
  ASSERT_TRUE(table.table_data);

  const auto& body_rows = table.table_data->body_rows;
  EXPECT_EQ(body_rows.size(), 4u);

  const auto& row_1 = body_rows[0]->cells;
  EXPECT_EQ(row_1.size(), 2u);
  EXPECT_EQ(row_1[0]->content_attributes->text_info[0]->text_content,
            "Personal Info");
  EXPECT_EQ(row_1[1]->content_attributes->text_info[0]->text_content,
            "Contact Info");

  const auto& row_2 = body_rows[1]->cells;
  EXPECT_EQ(row_2.size(), 4u);
  EXPECT_EQ(row_2[0]->content_attributes->text_info[0]->text_content, "Name");
  EXPECT_EQ(row_2[1]->content_attributes->text_info[0]->text_content, "Age");
  EXPECT_EQ(row_2[2]->content_attributes->text_info[0]->text_content, "Email");
  EXPECT_EQ(row_2[3]->content_attributes->text_info[0]->text_content, "Phone");

  const auto& row_3 = body_rows[2]->cells;
  EXPECT_EQ(row_3.size(), 4u);
  EXPECT_EQ(row_3[0]->content_attributes->text_info[0]->text_content,
            "John Doe");
  EXPECT_EQ(row_3[1]->content_attributes->text_info[0]->text_content, "30");
  EXPECT_EQ(row_3[2]->content_attributes->text_info[0]->text_content,
            "john.doe@example.com");
  EXPECT_EQ(row_3[3]->content_attributes->text_info[0]->text_content,
            "123-456-7890");

  const auto& row_4 = body_rows[3]->cells;
  EXPECT_EQ(row_4.size(), 4u);
  EXPECT_EQ(row_4[0]->content_attributes->text_info[0]->text_content,
            "Jane Smith");
  EXPECT_EQ(row_4[1]->content_attributes->text_info[0]->text_content, "28");
  EXPECT_EQ(row_4[2]->content_attributes->text_info[0]->text_content,
            "jane.smith@example.com");
  EXPECT_EQ(row_4[3]->content_attributes->text_info[0]->text_content,
            "987-654-3210");
}

TEST_F(AIPageContentAgentTest, LandmarkSections) {
  frame_test_helpers::LoadHTMLString(
      helper_.LocalMainFrame(),
      "<body>"
      "  <header>Header</header>"
      "  <nav>Navigation</nav>"
      "  <search>Search</search>"
      "  <main>Main content</main>"
      "  <article>Article</article>"
      "  <section>Section</section>"
      "  <aside>Aside</aside>"
      "  <footer>Footer</footer>"
      "</body>",
      url_test_helpers::ToKURL("http://foobar.com"));

  auto* agent = AIPageContentAgent::GetOrCreateForTesting(
      *helper_.LocalMainFrame()->GetFrame()->GetDocument());
  ASSERT_TRUE(agent);

  auto content = agent->GetAIPageContentSync();
  ASSERT_TRUE(content);
  ASSERT_TRUE(content->root_node);

  const auto& root = *content->root_node;
  ASSERT_EQ(root.children_nodes.size(), 8u);

  const auto& header = *root.children_nodes[0]->content_attributes;
  EXPECT_EQ(header.attribute_type,
            mojom::blink::AIPageContentAttributeType::kHeader);
  ASSERT_EQ(header.text_info.size(), 1u);
  EXPECT_EQ(header.text_info[0]->text_content, "Header");

  const auto& nav = *root.children_nodes[1]->content_attributes;
  EXPECT_EQ(nav.attribute_type, mojom::blink::AIPageContentAttributeType::kNav);
  ASSERT_EQ(nav.text_info.size(), 1u);
  EXPECT_EQ(nav.text_info[0]->text_content, "Navigation");

  const auto& search = *root.children_nodes[2]->content_attributes;
  EXPECT_EQ(search.attribute_type,
            mojom::blink::AIPageContentAttributeType::kSearch);
  ASSERT_EQ(search.text_info.size(), 1u);
  EXPECT_EQ(search.text_info[0]->text_content, "Search");

  const auto& main = *root.children_nodes[3]->content_attributes;
  EXPECT_EQ(main.attribute_type,
            mojom::blink::AIPageContentAttributeType::kMain);
  ASSERT_EQ(main.text_info.size(), 1u);
  EXPECT_EQ(main.text_info[0]->text_content, "Main content");

  const auto& article = *root.children_nodes[4]->content_attributes;
  EXPECT_EQ(article.attribute_type,
            mojom::blink::AIPageContentAttributeType::kArticle);
  ASSERT_EQ(article.text_info.size(), 1u);
  EXPECT_EQ(article.text_info[0]->text_content, "Article");

  const auto& section = *root.children_nodes[5]->content_attributes;
  EXPECT_EQ(section.attribute_type,
            mojom::blink::AIPageContentAttributeType::kSection);
  ASSERT_EQ(section.text_info.size(), 1u);
  EXPECT_EQ(section.text_info[0]->text_content, "Section");

  const auto& aside = *root.children_nodes[6]->content_attributes;
  EXPECT_EQ(aside.attribute_type,
            mojom::blink::AIPageContentAttributeType::kAside);
  ASSERT_EQ(aside.text_info.size(), 1u);
  EXPECT_EQ(aside.text_info[0]->text_content, "Aside");

  const auto& footer = *root.children_nodes[7]->content_attributes;
  EXPECT_EQ(footer.attribute_type,
            mojom::blink::AIPageContentAttributeType::kFooter);
  ASSERT_EQ(footer.text_info.size(), 1u);
  EXPECT_EQ(footer.text_info[0]->text_content, "Footer");
}

TEST_F(AIPageContentAgentTest, LandmarkSectionsWithAriaRoles) {
  frame_test_helpers::LoadHTMLString(
      helper_.LocalMainFrame(),
      "<body>"
      "  <div role='banner'>Header</div>"
      "  <div role='navigation'>Navigation</div>"
      "  <div role='search'>Search</div>"
      "  <div role='main'>Main content</div>"
      "  <div role='article'>Article</div>"
      "  <div role='region'>Section</div>"
      "  <div role='complementary'>Aside</div>"
      "  <div role='contentinfo'>Footer</div>"
      "</body>",
      url_test_helpers::ToKURL("http://foobar.com"));

  auto* agent = AIPageContentAgent::GetOrCreateForTesting(
      *helper_.LocalMainFrame()->GetFrame()->GetDocument());
  ASSERT_TRUE(agent);

  auto content = agent->GetAIPageContentSync();
  ASSERT_TRUE(content);
  ASSERT_TRUE(content->root_node);

  const auto& root = *content->root_node;
  ASSERT_EQ(root.children_nodes.size(), 8u);

  const auto& header = *root.children_nodes[0]->content_attributes;
  EXPECT_EQ(header.attribute_type,
            mojom::blink::AIPageContentAttributeType::kHeader);
  ASSERT_EQ(header.text_info.size(), 1u);
  EXPECT_EQ(header.text_info[0]->text_content, "Header");

  const auto& nav = *root.children_nodes[1]->content_attributes;
  EXPECT_EQ(nav.attribute_type, mojom::blink::AIPageContentAttributeType::kNav);
  ASSERT_EQ(nav.text_info.size(), 1u);
  EXPECT_EQ(nav.text_info[0]->text_content, "Navigation");

  const auto& search = *root.children_nodes[2]->content_attributes;
  EXPECT_EQ(search.attribute_type,
            mojom::blink::AIPageContentAttributeType::kSearch);
  ASSERT_EQ(search.text_info.size(), 1u);
  EXPECT_EQ(search.text_info[0]->text_content, "Search");

  const auto& main = *root.children_nodes[3]->content_attributes;
  EXPECT_EQ(main.attribute_type,
            mojom::blink::AIPageContentAttributeType::kMain);
  ASSERT_EQ(main.text_info.size(), 1u);
  EXPECT_EQ(main.text_info[0]->text_content, "Main content");

  const auto& article = *root.children_nodes[4]->content_attributes;
  EXPECT_EQ(article.attribute_type,
            mojom::blink::AIPageContentAttributeType::kArticle);
  ASSERT_EQ(article.text_info.size(), 1u);
  EXPECT_EQ(article.text_info[0]->text_content, "Article");

  const auto& section = *root.children_nodes[5]->content_attributes;
  EXPECT_EQ(section.attribute_type,
            mojom::blink::AIPageContentAttributeType::kSection);
  ASSERT_EQ(section.text_info.size(), 1u);
  EXPECT_EQ(section.text_info[0]->text_content, "Section");

  const auto& aside = *root.children_nodes[6]->content_attributes;
  EXPECT_EQ(aside.attribute_type,
            mojom::blink::AIPageContentAttributeType::kAside);
  ASSERT_EQ(aside.text_info.size(), 1u);
  EXPECT_EQ(aside.text_info[0]->text_content, "Aside");

  const auto& footer = *root.children_nodes[7]->content_attributes;
  EXPECT_EQ(footer.attribute_type,
            mojom::blink::AIPageContentAttributeType::kFooter);
  ASSERT_EQ(footer.text_info.size(), 1u);
  EXPECT_EQ(footer.text_info[0]->text_content, "Footer");
}

TEST_F(AIPageContentAgentTest, FixedPosition) {
  frame_test_helpers::LoadHTMLString(
      helper_.LocalMainFrame(),
      "     <body>"
      "       <style>"
      "       .fixed {"
      "         position: fixed;"
      "         top: 50px;"
      "         left: 50px;"
      "         width: 200px;"
      "       }"
      "       .sticky {"
      "         position: sticky;"
      "         top: 50px;"
      "         left: 3000px;"
      "         width: 200px;"
      "       }"
      "       .normal {"
      "         width: 250px;"
      "         height: 80px;"
      "         margin-top: 20px;"
      "       }"
      "       </style>"
      "       <div class='fixed'>"
      "         This element stays in place when the page is scrolled."
      "       </div>"
      "       <div class='sticky'>"
      "         This element stays in place when the page is scrolled."
      "       </div>"
      "       <div class='normal'>"
      "         This element flows naturally with the document."
      "       </div>"
      "     </body>",
      url_test_helpers::ToKURL("http://foobar.com"));

  auto* agent = AIPageContentAgent::GetOrCreateForTesting(
      *helper_.LocalMainFrame()->GetFrame()->GetDocument());
  ASSERT_TRUE(agent);

  auto content = agent->GetAIPageContentSync();
  ASSERT_TRUE(content);
  ASSERT_TRUE(content->root_node);

  const auto& root = *content->root_node;
  ASSERT_EQ(root.children_nodes.size(), 2u);

  // The normal element's text is part of the root node's text info.
  EXPECT_FALSE(root.content_attributes->geometry->is_fixed_or_sticky_position);
  EXPECT_EQ(
      root.content_attributes->text_info[0]->text_content.SimplifyWhiteSpace(),
      "This element flows naturally with the document.");

  const auto& fixed_element = *root.children_nodes[0]->content_attributes;
  EXPECT_EQ(fixed_element.attribute_type,
            mojom::blink::AIPageContentAttributeType::kContainer);
  EXPECT_TRUE(fixed_element.geometry->is_fixed_or_sticky_position);
  EXPECT_FALSE(fixed_element.geometry->scrolls_overflow_x);
  EXPECT_FALSE(fixed_element.geometry->scrolls_overflow_y);
  EXPECT_EQ(fixed_element.text_info[0]->text_content.SimplifyWhiteSpace(),
            "This element stays in place when the page is scrolled.");

  const auto& sticky_element = *root.children_nodes[1]->content_attributes;
  EXPECT_EQ(sticky_element.attribute_type,
            mojom::blink::AIPageContentAttributeType::kContainer);
  EXPECT_TRUE(sticky_element.geometry->is_fixed_or_sticky_position);
  EXPECT_FALSE(sticky_element.geometry->scrolls_overflow_x);
  EXPECT_FALSE(sticky_element.geometry->scrolls_overflow_y);
  EXPECT_EQ(sticky_element.text_info[0]->text_content.SimplifyWhiteSpace(),
            "This element stays in place when the page is scrolled.");
}

TEST_F(AIPageContentAgentTest, ScrollContainer) {
  frame_test_helpers::LoadHTMLString(
      helper_.LocalMainFrame(),
      "     <body>"
      "       <style>"
      "       .scrollable-x {"
      "         width: 100px;"
      "         height: 50px;"
      "         overflow-x: scroll;"
      "         overflow-y: clip;"
      "       }"
      "       .scrollable-y {"
      "         width: 300px;"
      "         height: 50px;"
      "         overflow-x: clip;"
      "         overflow-y: scroll;"
      "       }"
      "       .auto-scroll-x {"
      "         width: 100px;"
      "         height: 50px;"
      "         overflow-x: auto;"
      "         overflow-y: clip;"
      "       }"
      "       .auto-scroll-y {"
      "         width: 300px;"
      "         height: 50px;"
      "         overflow-x: clip;"
      "         overflow-y: auto;"
      "       }"
      "       .normal {"
      "         width: 250px;"
      "         height: 80px;"
      "         margin-top: 20px;"
      "       }"
      "       </style>"
      "       <div class='scrollable-x'>"
      "ABCDEFGHIJKLMOPQRSTUVWXYZABCDEFGHIJKLMOPQRSTUVWXYZABCDEFGHIJKLMOPQRSTUVW"
      "XYZABCDEFGHIJKLMOPQRSTUVWXYZABCDEFGHIJKLMOPQRSTUVWXYZABCDEFGHIJKLMOPQRST"
      "UVWXYZ"
      "       </div>"
      "       <div class='scrollable-y'>"
      "         Some long text to make it scrollable."
      "         Some long text to make it scrollable."
      "         Some long text to make it scrollable."
      "         Some long text to make it scrollable."
      "       </div>"
      "       <div class='auto-scroll-x'>"
      "ABCDEFGHIJKLMOPQRSTUVWXYZABCDEFGHIJKLMOPQRSTUVWXYZABCDEFGHIJKLMOPQRSTUVW"
      "XYZABCDEFGHIJKLMOPQRSTUVWXYZABCDEFGHIJKLMOPQRSTUVWXYZABCDEFGHIJKLMOPQRST"
      "UVWXYZ"
      "       </div>"
      "       <div class='auto-scroll-y'>"
      "         Some long text to make it scrollable."
      "         Some long text to make it scrollable."
      "         Some long text to make it scrollable."
      "         Some long text to make it scrollable."
      "       </div>"
      "     </body>",
      url_test_helpers::ToKURL("http://foobar.com"));

  auto* agent = AIPageContentAgent::GetOrCreateForTesting(
      *helper_.LocalMainFrame()->GetFrame()->GetDocument());
  ASSERT_TRUE(agent);

  auto content = agent->GetAIPageContentSync();
  ASSERT_TRUE(content);
  ASSERT_TRUE(content->root_node);

  const auto& root = *content->root_node;
  ASSERT_EQ(root.children_nodes.size(), 4u);

  EXPECT_TRUE(root.content_attributes->geometry->scrolls_overflow_x);
  EXPECT_TRUE(root.content_attributes->geometry->scrolls_overflow_y);

  const auto& scrollable_x_element =
      *root.children_nodes[0]->content_attributes;
  EXPECT_EQ(scrollable_x_element.attribute_type,
            mojom::blink::AIPageContentAttributeType::kContainer);
  EXPECT_FALSE(scrollable_x_element.geometry->is_fixed_or_sticky_position);
  EXPECT_TRUE(scrollable_x_element.geometry->scrolls_overflow_x);
  EXPECT_FALSE(scrollable_x_element.geometry->scrolls_overflow_y);
  EXPECT_EQ(
      scrollable_x_element.text_info[0]->text_content.SimplifyWhiteSpace(),
      "ABCDEFGHIJKLMOPQRSTUVWXYZABCDEFGHIJKLMOPQRSTUVWXYZABCDEFGHIJKLMOPQRSTUVW"
      "XYZABCDEFGHIJKLMOPQRSTUVWXYZABCDEFGHIJKLMOPQRSTUVWXYZABCDEFGHIJKLMOPQRST"
      "UVWXYZ");

  const auto& scrollable_y_element =
      *root.children_nodes[1]->content_attributes;
  EXPECT_EQ(scrollable_y_element.attribute_type,
            mojom::blink::AIPageContentAttributeType::kContainer);
  EXPECT_FALSE(scrollable_y_element.geometry->is_fixed_or_sticky_position);
  EXPECT_FALSE(scrollable_y_element.geometry->scrolls_overflow_x);
  EXPECT_TRUE(scrollable_y_element.geometry->scrolls_overflow_y);
  EXPECT_EQ(scrollable_y_element.text_info[0]->text_content.SimplifyWhiteSpace(),
            "Some long text to make it scrollable. Some long text to make it "
            "scrollable. Some long text to make it scrollable. Some long text "
            "to make it scrollable.");

  const auto& auto_scroll_x_element =
      *root.children_nodes[2]->content_attributes;
  EXPECT_EQ(auto_scroll_x_element.attribute_type,
            mojom::blink::AIPageContentAttributeType::kContainer);
  EXPECT_FALSE(auto_scroll_x_element.geometry->is_fixed_or_sticky_position);
  EXPECT_TRUE(auto_scroll_x_element.geometry->scrolls_overflow_x);
  EXPECT_FALSE(auto_scroll_x_element.geometry->scrolls_overflow_y);
  EXPECT_EQ(
      auto_scroll_x_element.text_info[0]->text_content.SimplifyWhiteSpace(),
      "ABCDEFGHIJKLMOPQRSTUVWXYZABCDEFGHIJKLMOPQRSTUVWXYZABCDEFGHIJKLMOPQRSTUVW"
      "XYZABCDEFGHIJKLMOPQRSTUVWXYZABCDEFGHIJKLMOPQRSTUVWXYZABCDEFGHIJKLMOPQRST"
      "UVWXYZ");

  const auto& auto_scroll_y_element =
      *root.children_nodes[3]->content_attributes;
  EXPECT_EQ(auto_scroll_y_element.attribute_type,
            mojom::blink::AIPageContentAttributeType::kContainer);
  EXPECT_FALSE(auto_scroll_y_element.geometry->is_fixed_or_sticky_position);
  EXPECT_FALSE(auto_scroll_y_element.geometry->scrolls_overflow_x);
  EXPECT_TRUE(auto_scroll_y_element.geometry->scrolls_overflow_y);
  EXPECT_EQ(
      auto_scroll_y_element.text_info[0]->text_content.SimplifyWhiteSpace(),
      "Some long text to make it scrollable. Some long text to make it "
      "scrollable. Some long text to make it scrollable. Some long text "
      "to make it scrollable.");
}

TEST_F(AIPageContentAgentTest, Links) {
  frame_test_helpers::LoadHTMLString(
      helper_.LocalMainFrame(),
      "<body>"
      "  <a href='https://www.google.com'>Google</a>"
      "  <a href='https://www.youtube.com' rel='noopener "
      "noreferrer'>YouTube</a>"
      "</body>",
      url_test_helpers::ToKURL("http://foobar.com"));

  auto* agent = AIPageContentAgent::GetOrCreateForTesting(
      *helper_.LocalMainFrame()->GetFrame()->GetDocument());
  ASSERT_TRUE(agent);

  auto content = agent->GetAIPageContentSync();
  ASSERT_TRUE(content);
  ASSERT_TRUE(content->root_node);

  const auto& root = *content->root_node;
  ASSERT_EQ(root.children_nodes.size(), 2u);

  const auto& link = *root.children_nodes[0]->content_attributes;
  EXPECT_EQ(link.attribute_type,
            mojom::blink::AIPageContentAttributeType::kAnchor);
  ASSERT_EQ(link.text_info.size(), 1u);
  EXPECT_EQ(link.text_info[0]->text_content, "Google");
  const auto* link_anchor_data = link.anchor_data.get();
  EXPECT_EQ(link_anchor_data->url, blink::KURL("https://www.google.com/"));
  EXPECT_EQ(link_anchor_data->rel.size(), 0u);

  const auto& link_with_rel = *root.children_nodes[1]->content_attributes;
  EXPECT_EQ(link_with_rel.attribute_type,
            mojom::blink::AIPageContentAttributeType::kAnchor);
  ASSERT_EQ(link_with_rel.text_info.size(), 1u);
  EXPECT_EQ(link_with_rel.text_info[0]->text_content, "YouTube");
  const auto* link_with_rel_anchor_data = link_with_rel.anchor_data.get();
  EXPECT_EQ(link_with_rel_anchor_data->url,
            blink::KURL("https://www.youtube.com/"));
  ASSERT_EQ(link_with_rel_anchor_data->rel.size(), 2u);
  EXPECT_EQ(link_with_rel_anchor_data->rel[0],
            mojom::blink::AIPageContentAnchorRel::kRelationNoOpener);
  EXPECT_EQ(link_with_rel_anchor_data->rel[1],
            mojom::blink::AIPageContentAnchorRel::kRelationNoReferrer);
}

}  // namespace
}  // namespace blink
