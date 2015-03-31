#include "../src/condtree.h"
#include <memory>

#ifndef BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE CondTree
#endif
#include <boost/test/unit_test.hpp>
#include <memory>

BOOST_AUTO_TEST_SUITE()

    BOOST_AUTO_TEST_CASE(Ctor) {
        BOOST_CHECK(CondTreeNode().type == CondTreeNode::Type::INVALID_NODE);
        BOOST_CHECK(CondTreeNode('+').type == CondTreeNode::Type::OP_NODE);
        BOOST_CHECK(CondTreeNode(0).type == CondTreeNode::Type::ID_NODE);
        BOOST_CHECK(CondTreeNode(3.14).type == CondTreeNode::Type::NUM_NODE);
    }

    BOOST_AUTO_TEST_CASE(CopyCtor) {
        auto root = CondTreeNode::make('*');
        root->left = CondTreeNode::make(1);
        root->right = CondTreeNode::make(2);
        auto c = root->clone();
        root->left = CondTreeNode::make('-');
        BOOST_CHECK(!root->isValid());
        BOOST_CHECK(c->isValid());
    }

    BOOST_AUTO_TEST_CASE(CtorTree) {
        BOOST_CHECK_NO_THROW(CondTree());
        auto x = CondTree();
        auto y = x;
        BOOST_CHECK(x.root);
        auto z = std::move(x);
        BOOST_CHECK(!x.root);
    }

    BOOST_AUTO_TEST_CASE(IsTerm_1) {
        const auto root = CondTreeNode();
        BOOST_CHECK_EQUAL(root.isTerm(), true);
    }

    BOOST_AUTO_TEST_CASE(IsTerm_2) {
        auto root = CondTreeNode::make('*');
        root->left = CondTreeNode::make(1);
        root->right = CondTreeNode::make(2);
        BOOST_CHECK_EQUAL(root->isTerm(), false);
    }

    BOOST_AUTO_TEST_CASE(IsValid) {
        auto root = CondTreeNode::make('*');
        BOOST_CHECK(!root->isValid());
        root->left = CondTreeNode::make(1);
        BOOST_CHECK(!root->isValid());
        root->right = CondTreeNode::make(2);
        BOOST_CHECK(root->isValid());
        root->left = CondTreeNode::make('+');
        BOOST_CHECK(!root->isValid());
        root->left->left = CondTreeNode::make(1.2);
        BOOST_CHECK(!root->isValid());
        root->left->right = CondTreeNode::make(2);
        BOOST_CHECK(root->isValid());
        root->right = CondTreeNode::make('-');
        BOOST_CHECK(!root->isValid());
        root->right = CondTreeNode::make(-1.0);
        BOOST_CHECK(root->isValid());
    }

BOOST_AUTO_TEST_SUITE_END()